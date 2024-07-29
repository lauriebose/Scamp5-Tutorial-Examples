#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

void DREG_load_centered_rect(dreg_t reg, int x, int y, int width, int height);

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

    int disp_size = 2;
    auto display_00 = vs_gui_add_display("A",0,0,disp_size);
    auto display_01 = vs_gui_add_display("S1",0,disp_size,disp_size);
    auto display_02 = vs_gui_add_display("WHERE(S1) B = A ",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

    int box1_x, box1_y, box1_width, box1_height;
    vs_gui_add_slider("box1 x: ", 0, 255, 64, &box1_x);
    vs_gui_add_slider("box1 y: ", 0, 255, 64, &box1_y);
    vs_gui_add_slider("box1 width: ", 0, 128, 96, &box1_width);
    vs_gui_add_slider("box1 height: ", 0, 128, 64, &box1_height);

    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//capture new image into AREG A

			scamp5_kernel_begin();
				get_image(A,E);
			scamp5_kernel_end();

    	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//load box shape into DREG S1

			DREG_load_centered_rect(S1,box1_x,box1_y,box1_width,box1_height);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//Example of conditional execution of AREG instructions using FLAG register

scamp5_kernel_begin();
	//copies S1 into FLAG, Identical to "MOV(FLAG,S1)"
	WHERE(S1);

		//AREG instructions are only performed in PEs where FLAG == 1
		//Thus "mov(B,A)" will now copy A, into B, ONLY where S1 == 1
		mov(B,A);

	//sets FLAG = 1 in all PEs, Identical to "SET(FLAG)"
	ALL();
scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT RESULTS STORED IN VARIOUS REGISTERS

	    	output_timer.reset();
	    	output_4bit_image_via_DNEWS(A,display_00);
			scamp5_output_image(S1,display_01);
			output_4bit_image_via_DNEWS(B,display_02);
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


void DREG_load_centered_rect(dreg_t reg, int x, int y, int width, int height)
{
	int top_row = y-height/2;
	if(top_row < 0)
	{
		height += top_row;
		top_row = 0;
	}
	int right_column = x-width/2;
	if(right_column < 0)
	{
		width += right_column;
		right_column = 0;
	}
	int bottom_row = top_row+height;
	int left_column = right_column+width;
	scamp5_load_rect(reg, top_row, right_column, bottom_row, left_column);
}

