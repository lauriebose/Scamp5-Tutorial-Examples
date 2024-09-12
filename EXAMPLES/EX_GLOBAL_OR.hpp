#include <scamp5.hpp>
using namespace SCAMP5_PE;
#include "MISC/MISC_FUNCS.hpp"

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();
//   	setup_voltage_configurator();//GLOBAL OR FLASHES IF THIS IS NOT HERE????!!!!

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("S0 (Thresholded Image)",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("S1 (box2)",0,display_size,display_size);
    vs_handle display_10 = vs_gui_add_display("S3 = S0 AND S1",display_size,0,display_size);
    vs_handle display_11 = vs_gui_add_display("GLOBAL OR S3 INDICATOR",display_size,display_size,display_size);

	int threshold_value = 64;
	vs_gui_add_slider("threshold_value",-127,127,threshold_value,&threshold_value);

    int box_x, box_y, box_width, box_height;
    vs_gui_add_slider("box x: ", 0, 255, 128, &box_x);
    vs_gui_add_slider("box y: ", 0, 255, 128, &box_y);
    vs_gui_add_slider("box width: ", 0, 128, 64, &box_width);
    vs_gui_add_slider("box height: ", 0, 128, 64, &box_height);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		 //CAPTURE FRAME AND PERFORM THRESHOLDING

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
      	//LOAD RECTANGULR REGION INTO S1

			DREG_load_centered_rect(S1,box_x,box_y,box_width,box_height);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //DEMONSTRATE GLOBAL_OR

			scamp5_kernel_begin();
				AND(S3,S0,S1); //S3 = the thresholded image contained inside the loaded rectangle
			scamp5_kernel_end();

			int global_or_value = scamp5_global_or(S3);//check if there are any PEs in which S3 = 1
			bool white_pixels_inside_rect  = global_or_value > 0 ? true : false;

			if(white_pixels_inside_rect )
			{
			   vs_post_text("global_or : TRUE, Value : %d\n",global_or_value);
			   scamp5_kernel_begin();
					SET(S2);
				scamp5_kernel_end();
			}
			else
			{
				vs_post_text("global_or : FALSE, Value : %d\n",global_or_value);
				scamp5_kernel_begin();
					CLR(S2);
				scamp5_kernel_end();
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			 //show DREG of box & box2
			scamp5_output_image(S0,display_00);
			scamp5_output_image(S1,display_01);
			scamp5_output_image(S3,display_10);//show OR result
			scamp5_output_image(S2,display_11);//show AND result
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

//		vs_post_text("box(%d,%d,%d,%d) box2(%d,%d,%d,%d)\n",box_x,box_y,box_width,box_height,box2_x,box2_y,box2_width,box2_height);

  		int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
  		int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
  		int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
        vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}

