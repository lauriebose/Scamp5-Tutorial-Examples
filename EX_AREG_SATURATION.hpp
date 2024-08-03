#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 1;
		auto display_00 = vs_gui_add_display("Original Image",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Step 1. A=A+Input",0,disp_size,disp_size);
		auto display_02 = vs_gui_add_display("Step 2. A=A+Input",0,2*disp_size,disp_size);
		auto display_03 = vs_gui_add_display("Step 3. A=A+Input",0,3*disp_size,disp_size);

		auto display_10 = vs_gui_add_display("AREG saturation error B-A",disp_size,0,disp_size);
		auto display_11 = vs_gui_add_display("Step 6. A=A-Input",disp_size,disp_size,disp_size);
		auto display_12 = vs_gui_add_display("Step 5. A=A-Input",disp_size,2*disp_size,disp_size);
		auto display_13 = vs_gui_add_display("Step 4. A=A-Input",disp_size,3*disp_size,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		int input_value = 64;
		vs_gui_add_slider("Input Value",-127,127,input_value,&input_value);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME

			scamp5_kernel_begin();
				get_image(B,F);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //ADD THE INPUT VALUE 3 TIMES, THEN SUBTRACT THE INPUT VALUE 3 TIMES TO DEMOSTRATE AREG SATURATION

			scamp5_kernel_begin();
				mov(A,B);//copy captured image into A
			scamp5_kernel_end();
			scamp5_in(C,input_value); //load input value into C across all PEs

			scamp5_kernel_begin();
				add(A,A,C);//Step 1. A = A + input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_01);

			scamp5_kernel_begin();
				add(A,A,C);//Step 2. A = A + input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_02);

			scamp5_kernel_begin();
				add(A,A,C);//Step 3. A = A + input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_03);

			scamp5_kernel_begin();
				sub(A,A,C);//Step 4. A = A - input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_13);

			scamp5_kernel_begin();
				sub(A,A,C);//Step 5. A = A - input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_12);

			scamp5_kernel_begin();
				sub(A,A,C);//Step 6. A = A - input
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(A,display_11);

			//after performing these additions and subtractions to AREG A its content should end up back where it started
			//however this is not necessarily true as the range of stored values in AREG is limited
			//a PE may have reached the maximum (or minimum) value for A during these additions/subtractions, effectively resulting in a loss of information
			scamp5_kernel_begin();
				sub(A,A,B);
				abs(C,A);//C == the absolute difference between A and the original image in B, i.e. the error due to AREG saturation
			scamp5_kernel_end();
			output_4bit_image_via_DNEWS(C,display_10);

			//display current captured frame and thresholded image
			output_4bit_image_via_DNEWS(B,display_00);

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			vs_post_text("frame time %d microseconds, potential FPS ~%d \n",frame_time_microseconds,max_possible_frame_rate); //display this values on host
    }
    return 0;
}

