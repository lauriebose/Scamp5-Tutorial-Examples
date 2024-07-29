#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Binary Thresholded Image",0,disp_size,disp_size);
		auto display_10 = vs_gui_add_display("Stored Image",disp_size,0,disp_size);
		auto display_11 = vs_gui_add_display("Stored Thresholded Image",disp_size,disp_size,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int threshold_value = 64;
		vs_gui_add_slider("threshold_value",-127,127,threshold_value,&threshold_value);

		auto gui_btn_store_images = vs_gui_add_button("Store Images");
		vs_on_gui_update(gui_btn_store_images,[&](int32_t new_value)
		{
			//When this gui button is pressed make copies of the current captured image and thresholded image
			scamp5_kernel_begin();
				mov(B,A);
				MOV(S1,S0);
			scamp5_kernel_end();
	   });

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME AND PERFORM THRESHOLDING

        	//load threshold value into C across all PEs
			scamp5_in(C,threshold_value);
			scamp5_kernel_begin();
				get_image(A,F);	//A = pixel data of latest frame, F = intermediate result
				sub(F,A,C);	//C = (A - C) == (latest frame pixel - threshold)
				where(F);	//sets FLAG = 1 in PEs where F > 0 (i.e. where A > C), else FLAG = 0
					//copy FLAG into S0
					MOV(S0,FLAG);
				all();
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

			output_timer.reset();

			//display current captured frame and thresholded image
			output_4bit_image_via_DNEWS(A,display_00);
			scamp5_output_image(S0,display_01);

			//display stored AREG / DREG images
			output_4bit_image_via_DNEWS(B,display_10);
			scamp5_output_image(S1,display_11);

			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}

