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
		auto display_01 = vs_gui_add_display("Shifted S0",0,disp_size,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    int kernel_selection = 0;
	    vs_gui_add_slider("kernel_selection",0,8,kernel_selection,&kernel_selection);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATE DATA IN DREG S0 TO DEMONSTRATE DNEWS OPERATION UPON

			//generate 4 boxes and 1 point in various DREG
			scamp5_load_point(S6,96,96);
			DREG_load_centered_rect(S5,188,118,32,32);
			DREG_load_centered_rect(S4,48,128,32,32);
			DREG_load_centered_rect(S3,10,125,10,200);
			DREG_load_centered_rect(S2,125,180,180,20);

			//combine S6,S5,S4 contents together in S0
			scamp5_kernel_begin();
				MOV(S0,S6);
				OR(S0,S5);
				OR(S0,S4);
				OR(S0,S3);
				OR(S0,S2);
			scamp5_kernel_end();

			scamp5_kernel_begin();
				MOV(S1,S0);
			scamp5_kernel_end();


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//shift content of S1 using DNEWS

			int time_spent_shifting = -1;

			scamp5_kernel_begin();
				CLR(RN,RS,RE,RW);//Clear all DREG controlling DNEWS behaviour
				SET(RW);
			scamp5_kernel_end();

			if(kernel_selection == 0)
			{
				vs_post_text("perform 100 kernels of length 2\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 100 ; n++)
				{
					scamp5_kernel_begin();
						DNEWS1(S6,S1);
						MOV(S1,S6);
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 1)
			{
				vs_post_text("perform 50 kernels of length 4\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 50 ; n++)
				{
					scamp5_kernel_begin();
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 2)
			{
				vs_post_text("perform 25 kernels of length 8\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 25 ; n++)
				{
					scamp5_kernel_begin();
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 3)
			{
				vs_post_text("perform 20 kernels of length 10\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 20 ; n++)
				{
					scamp5_kernel_begin();
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
						DNEWS1(S6,S1);
						MOV(S1,S6);
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}


			if(kernel_selection == 4)
			{
				vs_post_text("perform 10 kernels of length 20\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 10 ; n++)
				{
					scamp5_kernel_begin();
						//this loop is constant and never changes behaviour, and thus it is "safe" to put it inside of the kernel block
						//really this is just to avoid writing out the two instructions in the loop 10 times.... -_-
						for(int i = 0; i < 10 ; i++)
						{
							DNEWS1(S6,S1);
							MOV(S1,S6);
						}
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 5)
			{
				vs_post_text("perform 5 kernels of length 40\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 5 ; n++)
				{
					scamp5_kernel_begin();
						//loop to avoid writing this out 20 times... -__-
						for(int i = 0; i < 20 ; i++)
						{
							DNEWS1(S6,S1);
							MOV(S1,S6);
						}
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 6)
			{
				vs_post_text("perform 4 kernels of length 50\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 4 ; n++)
				{
					scamp5_kernel_begin();
						//loop to avoid writing this out 25 times... -__-
						for(int i = 0; i < 25 ; i++)
						{
							DNEWS1(S6,S1);
							MOV(S1,S6);
						}
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}


			if(kernel_selection == 7)
			{
				vs_post_text("perform 2 kernels of length 100\n");
				areg_shift_timer.reset();
				for(int n = 0 ; n < 2 ; n++)
				{
					scamp5_kernel_begin();
						//loop to avoid writing this out 50 times... -___-
						for(int i = 0; i < 50 ; i++)
						{
							DNEWS1(S6,S1);
							MOV(S1,S6);
						}
					scamp5_kernel_end();
				}
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			if(kernel_selection == 8)
			{
				vs_post_text("perform 1 kernel of length 200\n");
				areg_shift_timer.reset();
				scamp5_kernel_begin();
					//loop to avoid writing this out 100 times!
					for(int i = 0; i < 100 ; i++)
					{
						DNEWS1(S6,S1);
						MOV(S1,S6);
					}
				scamp5_kernel_end();
				time_spent_shifting = areg_shift_timer.get_usec();
			}

			vs_post_text("time spent shifting %d \n",time_spent_shifting);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			scamp5_output_image(S0,display_00);//display thresholded image
			scamp5_output_image(S1,display_01);//displayed shifted thresholded image
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


