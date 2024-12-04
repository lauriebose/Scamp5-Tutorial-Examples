#include <scamp5.hpp>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch kernel_test_timer;

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

	    int kernel_test_selection = 0;
	    vs_gui_add_slider("kernel selection",0,8,kernel_test_selection,&kernel_test_selection);

	    int binary_image_threshold = 0;
	    vs_gui_add_slider("binary image threshold",-127,127,binary_image_threshold,&binary_image_threshold);

	    int digital_kernels = 0;
	    vs_gui_add_switch("test digital kernels", digital_kernels == 1, &digital_kernels);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATE DATA IN DREG S0 TO DEMONSTRATE DNEWS OPERATION UPON

        scamp5_in(E,binary_image_threshold);
		scamp5_kernel_begin();
			get_image(A, F);	//get new frame
			mov(B,A);

			sub(F,A,E);
			where(F);
				MOV(S0,FLAG);
				MOV(S1,S0);
			all();
		scamp5_kernel_end();


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//TEST THE EXECUTION TIME TO PERFORM THE SAME TASK (SHIFTING DATA 100 PIXELS), USING KERNELS OF VARIOUS LENGTHS

			int time_spent_shifting = -1;

			 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Test using kernels containing only digital instructions
			if(digital_kernels == 1)
			{
				scamp5_kernel_begin();
					CLR(RN,RS,RE,RW);//Clear all DREG controlling DNEWS behaviour
					SET(RW);
				scamp5_kernel_end();

				if(kernel_test_selection == 0)
				{
					vs_post_text("Shift 100 pixels by performing 100 kernels of length 2\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 100 ; n++)
					{
						scamp5_kernel_begin();
							DNEWS1(S6,S1);
							MOV(S1,S6);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 1)
				{
					vs_post_text("Shift 100 pixels by performing 50 kernels of length 4\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 50 ; n++)
					{
						scamp5_kernel_begin();
							DNEWS1(S6,S1);
							MOV(S1,S6);
							DNEWS1(S6,S1);
							MOV(S1,S6);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 2)
				{
					vs_post_text("Shift 100 pixels by performing 25 kernels of length 8\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 3)
				{
					vs_post_text("Shift 100 pixels by performing 20 kernels of length 10\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}


				if(kernel_test_selection == 4)
				{
					vs_post_text("Shift 100 pixels by performing 10 kernels of length 20\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 5)
				{
					vs_post_text("Shift 100 pixels by performing 5 kernels of length 40\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 6)
				{
					vs_post_text("Shift 100 pixels by performing 4 kernels of length 50\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}


				if(kernel_test_selection == 7)
				{
					vs_post_text("Shift 100 pixels by performing 2 kernels of length 100\n");
					kernel_test_timer.reset();
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
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 8)
				{
					vs_post_text("Shift 100 pixels by performing 1 kernel of length 200\n");
					kernel_test_timer.reset();
					scamp5_kernel_begin();
						//loop to avoid writing this out 100 times!
						for(int i = 0; i < 100 ; i++)
						{
							DNEWS1(S6,S1);
							MOV(S1,S6);
						}
					scamp5_kernel_end();
					time_spent_shifting = kernel_test_timer.get_usec();
				}
			}


		   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Test using kernels containing only analogue instructions
			if(digital_kernels == 0)
			{

				if(kernel_test_selection == 0)
				{
					vs_post_text("Shift 100 pixels by performing 100 kernels of length 2\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 100 ; n++)
					{
						scamp5_kernel_begin();
							bus(NEWS,B);
							bus(B,XW);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 1)
				{
					vs_post_text("Shift 100 pixels by performing 50 kernels of length 4\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 50 ; n++)
					{
						scamp5_kernel_begin();
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 2)
				{
					vs_post_text("Shift 100 pixels by performing 25 kernels of length 8\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 25 ; n++)
					{
						scamp5_kernel_begin();
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 3)
				{
					vs_post_text("Shift 100 pixels by performing 20 kernels of length 10\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 20 ; n++)
					{
						scamp5_kernel_begin();
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
							bus(NEWS,B);
							bus(B,XW);
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}


				if(kernel_test_selection == 4)
				{
					vs_post_text("Shift 100 pixels by performing 10 kernels of length 20\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 10 ; n++)
					{
						scamp5_kernel_begin();
							//this loop is constant and never changes behaviour, and thus it is "safe" to put it inside of the kernel block
							//really this is just to avoid writing out the two instructions in the loop 10 times.... -_-
							for(int i = 0; i < 10 ; i++)
							{
								bus(NEWS,B);
								bus(B,XW);
							}
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 5)
				{
					vs_post_text("Shift 100 pixels by performing 5 kernels of length 40\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 5 ; n++)
					{
						scamp5_kernel_begin();
							//loop to avoid writing this out 20 times... -__-
							for(int i = 0; i < 20 ; i++)
							{
								bus(NEWS,B);
								bus(B,XW);
							}
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 6)
				{
					vs_post_text("Shift 100 pixels by performing 4 kernels of length 50\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 4 ; n++)
					{
						scamp5_kernel_begin();
							//loop to avoid writing this out 25 times... -__-
							for(int i = 0; i < 25 ; i++)
							{
								bus(NEWS,B);
								bus(B,XW);
							}
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}


				if(kernel_test_selection == 7)
				{
					vs_post_text("Shift 100 pixels by performing 2 kernels of length 100\n");
					kernel_test_timer.reset();
					for(int n = 0 ; n < 2 ; n++)
					{
						scamp5_kernel_begin();
							//loop to avoid writing this out 50 times... -___-
							for(int i = 0; i < 50 ; i++)
							{
								bus(NEWS,B);
								bus(B,XW);
							}
						scamp5_kernel_end();
					}
					time_spent_shifting = kernel_test_timer.get_usec();
				}

				if(kernel_test_selection == 8)
				{
					vs_post_text("Shift 100 pixels by performing 1 kernel of length 200\n");
					kernel_test_timer.reset();
					scamp5_kernel_begin();
						//loop to avoid writing this out 100 times!
						for(int i = 0; i < 100 ; i++)
						{
							bus(NEWS,B);
							bus(B,XW);
						}
					scamp5_kernel_end();
					time_spent_shifting = kernel_test_timer.get_usec();
				}
			}

			vs_post_text("time spent shifting %d \n",time_spent_shifting);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			if(digital_kernels)
			{
				scamp5_output_image(S0,display_00);//display thresholded image
				scamp5_output_image(S1,display_01);//displayed shifted thresholded image
			}
			else
			{
				scamp5_output_image(A,display_00);//display image
				scamp5_output_image(B,display_01);//displayed shifted image
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



