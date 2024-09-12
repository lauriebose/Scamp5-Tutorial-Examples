#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch scaling_timer;

int main()
{
    vs_init();

    int display_size = 2;
    auto display_00 = vs_gui_add_display("Captured Image",0,0,display_size);
    auto display_01 = vs_gui_add_display("RN",0,display_size,display_size);
    auto display_02 = vs_gui_add_display("RS",0,2*display_size,display_size);
    auto display_10 = vs_gui_add_display("Shifted Image Data",display_size,0,display_size);
    auto display_11 = vs_gui_add_display("Rows to copy data from S0",display_size,display_size,display_size);
    auto display_12 = vs_gui_add_display("Half Scaled Image",display_size,2*display_size,display_size);

    int scaling_steps = 64;
    vs_gui_add_slider("scaling_steps",0,64,scaling_steps,&scaling_steps);

    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//capture new image into AREG A and create copy in B

        	scamp5_in(C,-127);
			scamp5_kernel_begin();
				get_image(A,E);
				mov(B,A);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM VERTICAL HALF SCALING

			scaling_timer.reset();

			//setup the registers controlling DNEWS operation
	        scamp5_kernel_begin();
				CLR(RE,RW);
			scamp5_kernel_end();
	        DREG_load_centered_rect(RS,127,64,255,129);//Set RS in the top half of the PE array, will cause content in this half to move upwards
	        DREG_load_centered_rect(RN,127,196,255,129);//Set RN in the bottom half of the PE array, will cause content in this half to move downwards

	        //load horizontal line splitting the PE array into two halves
	        //S0 will give the rows of PEs at each step from which to copy shifted image data into the scaled result
	        DREG_load_centered_rect(S0,128,130,255,2);

			for(int x = 0; x < scaling_steps; x++)
			{
				//KERNEL SHIFTS B ONE "PIXE"L RIGHT
				scamp5_kernel_begin();

					WHERE(S0);
						mov(C,B);//copy rows of data for the current step into the AREG for the scaled result
					all();

					//shift image data in the top half of the PE array down 1 pixel
					WHERE(RN);
						bus(NEWS,B);
						bus(B,XS);
//					ALL();//Not needs as next analogue operation is WHERE

					//shift image data in the bottom half of the PE array up 1 pixel
					WHERE(RS);
						bus(NEWS,B);
						bus(B,XN);
					ALL();

					//perform DNEWS on S0 which stores the rows from which data is copied at each step
					//move the row of 1s in the top half up, and that in the bottom half down
					DNEWS0(S5,S0);
					MOV(S0,S5);
				scamp5_kernel_end();
			}

			int time_spent_scaling = scaling_timer.get_usec();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT REGISTERS AS IMAGES

        	output_timer.reset();
        	scamp5_output_image(RN,display_01);
			scamp5_output_image(RS,display_02);

			output_4bit_image_via_DNEWS(A,display_00);//captured image

			output_4bit_image_via_DNEWS(B,display_10);//shifted image data
			scamp5_output_image(S0,display_11);//rows from which to copy AREG data for current scaling step
			output_4bit_image_via_DNEWS(C,display_12);//scaled image data

			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

	        vs_post_text("time spent on half scaling %d microseconds\n",time_spent_scaling);

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}


