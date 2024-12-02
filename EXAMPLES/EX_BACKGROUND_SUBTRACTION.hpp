#include <scamp5.hpp>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_quantization_refresh_timer;

const areg_t AREG_captured_image = A;
const areg_t AREG_stored_background = B;
const areg_t AREG_background_update_mask = C;
const dreg_t DREG_detected_motion = S0;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Stored Background",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("Background Subtraction Result",0,disp_size*2,disp_size);
		auto display_10 = vs_gui_add_display("Background Update Mask",disp_size,0,disp_size);
		auto display_11 = vs_gui_add_display("Motion",disp_size,disp_size,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int quantization_levels = 16;
		vs_gui_add_slider("quantization_levels",2,32,quantization_levels,&quantization_levels);

		//Each frame the background update mask is steadily increased in value according to the following variables
		//The stored background is updated wherever this mask is high in value
		int bg_mask_increase_value = 10;//value added to mask
		vs_gui_add_slider("bg_mask_inc_value",0,127,bg_mask_increase_value,&bg_mask_increase_value);
		int bg_mask_increase_interval = 250;//time in ms between each addition
		vs_gui_add_slider("bg_mask_inc_interval",0,1000,bg_mask_increase_interval,&bg_mask_increase_interval);
		vs_stopwatch bg_mask_update_timer;

		//Threshold used to detect motion, using frame differencing (i.e. abs(current_frame-previous_frame)
		//The background update mask is reset wherever motion is detected, stopping the background from being updated in those PEs
		int motion_threshold = 16;
		vs_gui_add_slider("motion_threshold",0,127,motion_threshold,&motion_threshold);

		int motion_expansion = 5;
		vs_gui_add_slider("motion_expansion",0,20,motion_expansion,&motion_expansion);

		//The threshold used to detect differences between the stored background and the current frame
		int background_sub_threshold = 32;
		vs_gui_add_slider("background_sub_threshold",0,127,background_sub_threshold,&background_sub_threshold);

		auto gui_btn_store_images = vs_gui_add_button("Set Background");
		vs_on_gui_update(gui_btn_store_images,[&](int32_t new_value)
		{
			scamp5_in(AREG_background_update_mask,127); //Reset Background Mask
	   });

		int output_binary_result = 1;
		vs_gui_add_switch("binary_result",output_binary_result == 1,&output_binary_result);

		//Reset background update mask
		scamp5_in(AREG_background_update_mask,127);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME

			scamp5_kernel_begin();
				mov(E, AREG_captured_image);//store a copy previous frame
				get_image(AREG_captured_image, F);	//get new frame
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//DETECT MOTION BY FRAME DIFFERENCING

		 	scamp5_in(D,motion_threshold);
			scamp5_kernel_begin();
				//Compute absolute difference between latest frame and previous frame
				sub(F,AREG_captured_image,E);// E = difference between latest and previous frame
				abs(E,F);// F == absolute difference between current and last frame

				//Detect PEs where "motion" occurred due to absolute difference between current and last frame being above threshold
				sub(E,E,D);
				where(E);
					MOV(DREG_detected_motion,FLAG);//Store mask of PEs where there was "motion"
				all();
			scamp5_kernel_end();

			//Expand motion mask to ensure background is not updated in any PEs even near where motion is occuring
			scamp5_kernel_begin();
				SET(RN,RS,RE,RW);
			scamp5_kernel_end();
			for(int n = 0 ; n < motion_expansion; n++)
			{
				scamp5_kernel_begin();
					DNEWS0(S6,DREG_detected_motion);//S6 will contain 1s at locations that S0 will expand into this step
					OR(DREG_detected_motion,S6);//Combine expanded locations with S0 itself
				scamp5_kernel_end();
			}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//UPDATE BACKGROUND MASK

			//Where motion occurred reset background update mask to minimum value
			scamp5_in(D,-127);
			scamp5_kernel_begin();
				WHERE(DREG_detected_motion);
					mov(AREG_background_update_mask,D);
				all();
			scamp5_kernel_end();

			//If sufficient time has passed, increase the value of mask
			if(bg_mask_update_timer.get_usec() >= bg_mask_increase_interval*1000)
			{
				//Reset Counter
				bg_mask_update_timer.reset();

				//Update Background Update Mask
				scamp5_in(F,bg_mask_increase_value);
				scamp5_kernel_begin();
					add(AREG_background_update_mask,AREG_background_update_mask,F);
				scamp5_kernel_end();
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//UPDATE BACKGROUND, BUT ONLY WHERE BACKGROUND UPDATE MASK IS HIGH VALUE

			int background_update_mask_threshold = 100;
			scamp5_in(F,background_update_mask_threshold);//Threshold at which to perform background update
			scamp5_kernel_begin();
				sub(E,AREG_background_update_mask,F);
				where(E);//Where Background Update Mask > Threshold
					mov(AREG_stored_background,AREG_captured_image);//Update Background
				all();
			scamp5_kernel_end();

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
						sub(F,F,AREG_stored_background);
						where(F);//Compare stored data with decision value, FLAG == 1 in PEs with data below the decision value

							AND(S5,S6,FLAG);//Get PEs whose is below the decision value, AND has not been snapped yet, these PEs will be snapped to a value now
							NOT(S6,FLAG);//Update S6, copy the FLAG over now before the next WHERE instruction, all PEs where FLAG = 1 will have had their data snapped to a value

							WHERE(S5);
								mov(AREG_stored_background,E);//Snap the data of these PEs to the current quantization value
						all();
					scamp5_kernel_end();

					//update current and decision values
					current_value += quantization_interval;
					decision_value = current_value + quantization_interval/2;
				}

				//snap all remaining PEs that have not been quantized to the max quantization value
				scamp5_in(E,127);
				scamp5_kernel_begin();
					WHERE(S6);
						mov(AREG_stored_background,E);
					all();
				scamp5_kernel_end();
			}
			int time_spent_on_areg_quantization_refresh = areg_quantization_refresh_timer.get_usec();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE BACKGROUND SUBTRACTION MASK

			scamp5_in(F,background_sub_threshold);
			scamp5_kernel_begin();
				//Compute absolute difference between background and current frame
				sub(D,AREG_captured_image,AREG_stored_background);
				abs(E,D);

				//Find where absolute difference is above threshold, and copy the current frames data into these PEs
				sub(E,E,F);
				where(E);
					MOV(S5,FLAG);//Store binary background subtraction mask/result
				all();
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();

			//display current captured frame and thresholded image
			output_4bit_image_via_DNEWS(AREG_captured_image,display_00);
			output_4bit_image_via_DNEWS(AREG_stored_background,display_01);
			output_4bit_image_via_DNEWS(AREG_background_update_mask,display_10);
			scamp5_output_image(DREG_detected_motion,display_11);
			if(output_binary_result)
			{
				scamp5_output_image(S5,display_02);
			}
			else
			{
				//Generate analogue background subtraction image using the binary background subtraction mask
				scamp5_in(E,-127);
				scamp5_kernel_begin();
					WHERE(S5);
						mov(E,AREG_captured_image);
					ALL();
				scamp5_kernel_end();
				output_4bit_image_via_DNEWS(E,display_02);
			}

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

