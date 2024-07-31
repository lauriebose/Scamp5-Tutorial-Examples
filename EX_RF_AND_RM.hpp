#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_shift_timer;

void DREG_load_centered_rect(dreg_t dr, int centre_x, int centre_y, int width, int height);

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("S0",0,0,disp_size);
		auto display_01 = vs_gui_add_display("RF DIGITAL ""FLAG""",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("RP = S0 FLAGGED BY RF ",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    int box_x, box_y, box_width, box_height;
	    vs_gui_add_slider("box x: ", 0, 255, 128, &box_x);
	    vs_gui_add_slider("box y: ", 0, 255, 128, &box_y);
	    vs_gui_add_slider("box width: ", 0, 255, 96, &box_width);
	    vs_gui_add_slider("box height: ", 0, 255, 96, &box_height);

		int threshold_value = 64;
		vs_gui_add_slider("threshold_value",-127,127,threshold_value,&threshold_value);

		int clear_RP = 0;
		vs_gui_add_switch("clear_RP",clear_RP == 1, &clear_RP);

		int refresh_RP = 0;
		vs_gui_add_switch("refresh_RP",refresh_RP == 1, &refresh_RP);


    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//CAPTURE FRAME AND PERFORP THRESHOLDING

			//load threshold value into C across all PEs
			scamp5_in(C,threshold_value);

			scamp5_kernel_begin();
				//A = pixel data of latest frame, F = intermediate result
				get_image(A,F);

				//C = (A - C) == (latest frame pixel - threshold)
				sub(F,A,C);

				//sets FLAG = 1 in PEs where F > 0 (i.e. where A > C), else FLAG = 0
				where(F);
					//copy FLAG into S0
					MOV(S0,FLAG);
				all();
			scamp5_kernel_end();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //EXAMPLE OF PERFORPING "FLAGGED" DIGITAL OPERATIONS WITH RF & RP

			if(clear_RP)
			{
				scamp5_kernel_begin();
					//Set RF=1 in all PEs
					SET(RF);
						//Set RP=0 in all PEs where RF=1 (currently all of them!)
						CLR(RP);
				scamp5_kernel_end();
			}


			//Load a rectangular region into S6, this will be copied to RF later
			DREG_load_centered_rect(S6,box_x,box_y,box_width,box_height);

			//All digital operations targeting RP are flagged by RF
			//I.E. when a digital operation is performed that would change the content of RP, it is only performed in PEs where RF = 1
			scamp5_kernel_begin();
				//Copy rectangular region in S6 into FLAG
				MOV(RF,S6);

				    //Copy thresholded image, stored in S0, into RP.
				    //Will only be performed in PEs where RF=1 (i.e. only inside the rectangle)
					MOV(RP,S0);
			scamp5_kernel_end();

			if(refresh_RP)
			{
				scamp5_kernel_begin();
					SET(RF);
						REFRESH(RP);
				scamp5_kernel_end();
			}


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			scamp5_output_image(S0,display_00);
			scamp5_output_image(S6,display_01);
			scamp5_output_image(RP,display_02);
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

void DREG_load_centered_rect(dreg_t dr, int centre_x, int centre_y, int width, int height)
{
	int top_left_row = centre_y-height/2;
	if(top_left_row < 0)
	{
		height += top_left_row;
		top_left_row = 0;
	}
	int top_left_column = centre_x-width/2;
	if(top_left_column < 0)
	{
		width += top_left_column;
		top_left_column = 0;
	}

	scamp5_load_rect(dr, top_left_row, top_left_column, top_left_row+height, top_left_column+width);
}
