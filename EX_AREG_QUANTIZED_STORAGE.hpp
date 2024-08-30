#include <scamp5.hpp>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_quantization_refresh_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Stored Quantized Image",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("Stored Image",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int quantization_levels = 10;
		vs_gui_add_slider("quantization_levels",2,32,quantization_levels,&quantization_levels);

		bool store_image_trigger = false;
		auto gui_btn_store_images = vs_gui_add_button("Store Images");
		vs_on_gui_update(gui_btn_store_images,[&](int32_t new_value)
		{
			//When this gui button is pressed set trigger to store current image
			store_image_trigger = true;
	   });

		bool generate_test_image_trigger = false;
		auto gui_btn_gen_test_image = vs_gui_add_button("Gradient Test Image");
		vs_on_gui_update(gui_btn_gen_test_image,[&](int32_t new_value)
		{
			//When this gui button is pressed set trigger to generate and store a test image
			generate_test_image_trigger = true;
	   });

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME

			scamp5_kernel_begin();
				get_image(A,F);	//A = pixel data of latest frame, F = intermediate result
			scamp5_kernel_end();

			if(store_image_trigger)
			{
				store_image_trigger = false;

				scamp5_kernel_begin();
					mov(B,A);
					mov(C,A);
				scamp5_kernel_end();
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//GENERATE GRADIENT TEST IMAGE

			if(generate_test_image_trigger)
			{
				generate_test_image_trigger = false;

				//Generate a test image
				//A gradient image going from dark to bright, from top to bottom of the AREG plane

				//Load 1s into the top row of a DREG plane, will be used as a mask to input values to the generated image
				scamp5_load_pattern(S5,0,0,0,255);

				//Setup DREG controlling DNEWS direction for shifting the row of 1s downward
				scamp5_kernel_begin();
					CLR(RN,RS,RW,RE);
					SET(RN);
				scamp5_kernel_end();

				//Iterate across all rows of the PE array
				//At each row, input a different value into the AREG to form a gradient image across the AREG plane
				for(int n = 0 ; n < 255 ; n++)
				{
					scamp5_in(F,-127 + n);//Value to input, goes from dark to bright

					scamp5_kernel_begin();
						WHERE(S5);//In PEs belonging to the row of 1s
							mov(B,F);//Input the Value into the AREG of these PEs
							DNEWS0(S5,FLAG);//Shift the row of 1s downward
						all();//Reset FLAG
					scamp5_kernel_end();
				}

				scamp5_kernel_begin();
					mov(C,B);
				scamp5_kernel_end();
			}

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//REFRESH QUANTIZED AREG STORAGE, SNAPS EACH PE'S STORED VALUE TO THE NEAREST QUANTIZED VALUE TO PREVENT VALUE DECAY/DRIFT

			areg_quantization_refresh_timer.reset();
			{
				scamp5_kernel_begin();
					SET(S6);//Use the S6 DREG to track which PEs have been snapped to a quantization level
				scamp5_kernel_end();

				int quantization_interval = 256/(quantization_levels-1);
				int current_value = -127;//Value of the current quantization level
				int decision_value = current_value + quantization_interval/2;//Value to compare with data to determine if it should snap downwards to current quantization level

				//Iterate through each value of quantization, from lowest to highest
				for(int n = 0 ; n < quantization_levels-1 ;n++)
				{
					scamp5_in(F,decision_value);
					scamp5_in(E,current_value);

					scamp5_kernel_begin();
						sub(F,F,B);
						where(F);//Compare stored data with decision value, FLAG == 1 in PEs with data below the decision value

							AND(S5,S6,FLAG);//Get PEs whose is below the decision value, AND has not been snapped yet, these PEs will be snapped to a value now
							NOT(S6,FLAG);//Update S6, copy the FLAG over now before the next WHERE instruction, all PEs where FLAG = 1 will have had their data snapped to a value

							WHERE(S5);
								mov(B,E);//Snap the data of these PEs to the current quantization value
						all();
					scamp5_kernel_end();

					//update current and decision values
					current_value += quantization_interval;
					decision_value = current_value + quantization_interval/2;
				}

				scamp5_in(E,127);
				scamp5_kernel_begin();
					WHERE(S6);
						mov(B,E);
					all();
				scamp5_kernel_end();
			}
			int time_spent_on_areg_quantization_refresh = areg_quantization_refresh_timer.get_usec();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

			output_timer.reset();

			//display current captured frame and thresholded image
			output_4bit_image_via_DNEWS(A,display_00);
			output_4bit_image_via_DNEWS(B,display_01);
			output_4bit_image_via_DNEWS(C,display_02);

			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("time_spent_on_areg_quantization_refresh %d\n",time_spent_on_areg_quantization_refresh);
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}

