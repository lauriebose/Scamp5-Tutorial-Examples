#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;
vs_stopwatch frame_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS
		int disp_size = 2;
		vs_handle display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int image_output = 1;
		vs_gui_add_switch("image_output",1,&image_output);

		int use_4bit_image_output = 1;
		vs_gui_add_switch("use_4bit_image_output",1,&use_4bit_image_output);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer


        vs_frame_loop_control();
        vs_disable_frame_trigger();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME AND PERFORM AREG OPERATIONS

			scamp5_kernel_begin();
				//A = pixel data of latest frame, F = intermediate result
				get_image(A,F);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES
			if(image_output)
			{
				//output register plane as image
				if(use_4bit_image_output)
				{
					//output AREG quickly using 4bit approximation
					output_4bit_image_via_DNEWS(A,display_00);
				}
				else
				{
					//output AREG at higher accuracy but taking significantly longer
					scamp5_output_image(A,display_00);
				}
			}

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame

			vs_post_text("!clear");
			//calculate the possible max FPS
			int max_possible_frame_rate = 1000000/frame_time_microseconds;
			//display this values on host
			vs_post_text("frame time %d microseconds, potential FPS ~%d \n",frame_time_microseconds,max_possible_frame_rate);

    }
    return 0;
}

