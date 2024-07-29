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
		auto display_00 = vs_gui_add_display("Current Captured Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Previous Captured Image",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("Current Image - Previous Image",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int image_output = 1;
		vs_gui_add_switch("image_output",1,&image_output);

		int use_4bit_image_output = 1;
		vs_gui_add_switch("use_4bit_image_output",1,&use_4bit_image_output);

		int perform_abs = 0;
		vs_gui_add_switch("perform_abs",0,&perform_abs);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME AND PERFORM AREG OPERATIONS

			//Analogue Register (AREG) usage
			//A - stores the latest frame's pixels
			//B - stores the previous frame's pixels
			//C - stores the difference between frames A-B
			scamp5_kernel_begin();
			    //Each PE contains a PIX register which accumulates light entering the PE
			    //"get_image" copies this accumulated signal into an AREG
			    //This “captures” an image frame, with each PE storing 1 pixel of the image
		        //The accumulation within PIX is also reset


				//A = pixel data of latest frame, F = intermediate result
				get_image(A,F);

				//C = (A - C) == (latest frame pixel - previous frame pixel)
				sub(C,A,B);

				//B = A, makes a copy of the latest frame's pixel data before a new frame is acquired
				mov(B,A);
			scamp5_kernel_end();

			if(perform_abs)
			{
			   scamp5_kernel_begin();
					abs(F,C);//F = |C|
					mov(C,F);//copy the result in F back into C
				scamp5_kernel_end();
			}


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

output_timer.reset();
if(image_output)
{
	//display the contents of PE registers as images, displaying 3 images for A,B,C
	if(use_4bit_image_output)
	{
		//output AREG quickly using 4bit approximation
		output_4bit_image_via_DNEWS(A,display_00);
		output_4bit_image_via_DNEWS(B,display_01);
		output_4bit_image_via_DNEWS(C,display_02);
	}
	else
	{
		//output AREG slowly at higher accuracy
		scamp5_output_image(A,display_00);
		scamp5_output_image(B,display_01);
		scamp5_output_image(C,display_02);
	}
}
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

