#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_shift_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Binary Thresholded Image",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("Shifted Binary Thresholded Image",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int threshold_value = 64;
		vs_gui_add_slider("threshold_value",-127,127,threshold_value,&threshold_value);

		//for controlling the shift applied to the DREG content
	    int shift_x = 40;
	    vs_gui_add_slider("shift_x",-255,255,shift_x,&shift_x);
	    int shift_y = 40;
	    vs_gui_add_slider("shift_y",-255,255,shift_y,&shift_y);

		//for toggling between using "DNEWS0" or "DNEWS1"
		int use_DNEWS1 = 0;
		vs_gui_add_switch("use_DNEWS1",1,&use_DNEWS1);

		int output_just_shifted_img = 0;
		vs_gui_add_switch("output_just_shifted_img",output_just_shifted_img == 1,&output_just_shifted_img);

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

  				sub(F,A,C); //C = (A - C) == (latest frame pixel - threshold)

  				where(F);//sets FLAG = 1 in PEs where F > 0 (i.e. where A > C), else FLAG = 0

  					MOV(S0,FLAG);//copy FLAG into S0, will be performed on all PEs as FLAG does not effect Digital instruction execution, only analogue
  					MOV(S1,FLAG);
  				all();//Resets FLAG = 1 across all PEs
  			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//shift content of S1 using DNEWS

  			areg_shift_timer.reset();

  			//First shift horizontally
  			{
				scamp5_kernel_begin();
					CLR(RN,RS,RE,RW);//Clear all DREG controlling DNEWS behaviour
				scamp5_kernel_end();

				//set the appropriate DREG to shift horizontally left or right
				if(shift_x > 0)
				{
					scamp5_kernel_begin();
						SET(RW);
					scamp5_kernel_end();
				}
				else
				{
					scamp5_kernel_begin();
						SET(RE);
					scamp5_kernel_end();
				}

				//Now after setting up the RN,RS,RE,RW correctly, shift the content of S1 horizontally by repeatedly performing DNEWS operations
				int iteration = abs(shift_x);
				if(!use_DNEWS1)
				{
					for(int n = 0 ; n < iteration ; n++)
					{
						scamp5_kernel_begin();
							DNEWS0(S6,S1);//S6 = DNEWS(S0)
							MOV(S1,S6);//Copy the results in S6 back into S0
						scamp5_kernel_end();
					}
				}
				else
				{
					for(int n = 0 ; n < iteration ; n++)
					{
						scamp5_kernel_begin();
							DNEWS1(S6,S1);
							MOV(S1,S6);
						scamp5_kernel_end();
					}
				}
  			}

  			//Now also shift vertically
  			{
				scamp5_kernel_begin();
					CLR(RN,RS,RE,RW);//Clear all DREG controlling DNEWS behaviour
				scamp5_kernel_end();

				//set the appropriate DREG to shift vertically up or down
				if(shift_y > 0)
				{
					scamp5_kernel_begin();
						SET(RN);
					scamp5_kernel_end();
				}
				else
				{
					scamp5_kernel_begin();
						SET(RS);
					scamp5_kernel_end();
				}

				//Now after setting up the RN,RS,RE,RW correctly, shift the content of S1 vertically by repeatedly performing DNEWS operations
				int iteration = abs(shift_y);
				if(!use_DNEWS1)
				{
					for(int n = 0 ; n < iteration ; n++)
					{
						scamp5_kernel_begin();
							DNEWS0(S6,S1);
							MOV(S1,S6);
						scamp5_kernel_end();
					}
				}
				else
				{
					for(int n = 0 ; n < iteration ; n++)
					{
						scamp5_kernel_begin();
							DNEWS1(S6,S1);
							MOV(S1,S6);
						scamp5_kernel_end();
					}
				}
  			}

  			int time_spent_shifting = areg_shift_timer.get_usec();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			if(!output_just_shifted_img)
			{
				output_4bit_image_via_DNEWS(A,display_00);//output AREG quickly using 4bit approximation
				scamp5_output_image(S0,display_01);//display thresholded image
			}
			scamp5_output_image(S1,display_02);//displayed shifted thresholded image
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("time spent shifting %d \n",time_spent_shifting);
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}
