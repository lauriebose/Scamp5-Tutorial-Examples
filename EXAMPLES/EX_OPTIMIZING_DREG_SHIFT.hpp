#include <scamp5.hpp>
using namespace SCAMP5_PE;
void batched_shift_S5(int shiftx, int shifty);
void batched_shift_S5_no_decay(int shiftx, int shifty);
void simple_shift_S5(int shiftx, int shifty);

int main()
{
    vs_init();


	VS_GUI_DISPLAY_STYLE(style_plot,R"JSON(
			{
				"plot_palette": "plot_4",
				"plot_palette_groups": 4
			}
			)JSON");

    int display_size = 1;
    auto display_00 = vs_gui_add_display("scamp_shift()",0,0,display_size);
    auto display_01 = vs_gui_add_display("simple_shift",0,display_size,display_size);
    auto display_02 = vs_gui_add_display("batched_shift_no_decay",display_size,0,display_size);
    auto display_03 = vs_gui_add_display("batched_shift",display_size,display_size,display_size);
    auto display_04 = vs_gui_add_display("White:scamp_shift(),  Green:Simple,  Red:Batched(no decay),  Yellow:Batched",0,display_size*2,display_size*2,style_plot);
    vs_gui_set_scope(display_04,0,80,256);
    int threshold = 0;
    vs_gui_add_slider("threshold",-127,127,threshold,&threshold);

    int shiftx = 32;
    vs_gui_add_slider("shift x",-64,64,shiftx,&shiftx);
    int shifty = 0;
    vs_gui_add_slider("shift y",-64,64,shifty,&shifty);

    vs_stopwatch timer;

    // Frame Loop
    while(1){
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		//CAPTURE IMAGE

        scamp5_in(F,threshold);
        scamp5_kernel_begin();
        	get_image(A,E);

        	//Create thresholded image
        	sub(E,A,F);
        	where(E);
        		MOV(S0,FLAG);
        		MOV(S5,FLAG);
        	all();
        scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SHIFT USING scamp_shift()

        timer.reset();
        scamp5_shift(S5,shiftx,shifty);//This is very slow?
        int library_shift_time = timer.get_usec();
        scamp5_kernel_begin();
  			MOV(S1,S5);
  			MOV(S5,S0);
  	    scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SHIFT USING FOR LOOP

        timer.reset();
        simple_shift_S5(shiftx,shifty);
        int  simple_shift_time = timer.get_usec();
        scamp5_kernel_begin();
  			MOV(S2,S5);
  			MOV(S5,S0);
  	    scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//BATCHED SHIFT USING LESS KERNEL CALLS BY, AVOIDING SEQUENTIAL DNEWS INSTRUCTIONS AS THESE ARE ERROR PRONE ON SCAMP5

        timer.reset();
        batched_shift_S5_no_decay(shiftx,shifty);
        int batched_no_decay_shift_time = timer.get_usec();
        scamp5_kernel_begin();
  			MOV(S3,S5);
  			MOV(S5,S0);
  	    scamp5_kernel_end();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//BATCHED SHIFT USING LESS KERNEL CALLS BY

        timer.reset();
        batched_shift_S5(shiftx,shifty);
        int batched_shift_time = timer.get_usec();
        scamp5_kernel_begin();
  			MOV(S4,S5);
  	    scamp5_kernel_end();

  	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  	   //OUTPUT


  	    //Post data for plot of execution times of each method
  		int32_t plot_data[4];
  		plot_data[0] = batched_shift_time;
  		plot_data[1] = batched_no_decay_shift_time;
  		plot_data[2] = simple_shift_time;
  		plot_data[3] = library_shift_time;
		vs_post_set_channel(display_04);
		vs_post_int32(plot_data,1,4);

        vs_post_text("Execution Times : Batched Shift %d, Batched Shift no decay %d, Simple Shift %d, scamp_shift() %d \n",batched_shift_time,batched_no_decay_shift_time, simple_shift_time, library_shift_time);

		scamp5_output_image(S1,display_00);
		scamp5_output_image(S2,display_01);
		scamp5_output_image(S3,display_02);
		scamp5_output_image(S4,display_03);
    }

    return 0;
}


void perform_batched_DNEWS_iterations_S5_S6(int remaining_iterations)
{
	while(remaining_iterations > 0)
	{
		if(remaining_iterations % 8 == 0)//Check if remaining shift is divisible by 8
		{
			scamp5_kernel_begin();//Kernel to shift data 8 times
				DNEWS0(S6,S5);
				DNEWS0(S5,S6);
				DNEWS0(S6,S5);
				DNEWS0(S5,S6);
				DNEWS0(S6,S5);
				DNEWS0(S5,S6);
				DNEWS0(S6,S5);
				DNEWS0(S5,S6);
			scamp5_kernel_end();
			remaining_iterations -= 8;//Update remaining shift
		}
		else
		{
			if(remaining_iterations % 4 == 0)//Check if remaining shift is divisible by 4
			{
				scamp5_kernel_begin();//Kernel to shift data 4 times
					DNEWS0(S6,S5);
					DNEWS0(S5,S6);
					DNEWS0(S6,S5);
					DNEWS0(S5,S6);
				scamp5_kernel_end();
				remaining_iterations -= 4;//Update remaining shift
			}
			else
			{
				if(remaining_iterations % 2 == 0)//Check if remaining shift is divisible by 2
				{
					scamp5_kernel_begin();//Kernel to shift data 2 times
						DNEWS0(S6,S5);
						DNEWS0(S5,S6);
					scamp5_kernel_end();
					remaining_iterations -= 2;//Update remaining shift
				}
				else
				{
					scamp5_kernel_begin();//Kernel to shift data once
						DNEWS0(S6,S5);
						MOV(S5,S6);
					scamp5_kernel_end();
					remaining_iterations -= 1;//Update remaining shift
				}
			}
		}
	}
}

void batched_shift_S5(int shiftx, int shifty)
{
	if(shiftx > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RE);
			SET(RW);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6(shiftx);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RW);
			SET(RE);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6(-shiftx);//Call function to perform DNEWS instructions and shift data
	}

	if(shifty > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RS,RE,RW);
			SET(RN);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6(shifty);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RE,RW);
			SET(RS);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6(-shifty);//Call function to perform DNEWS instructions and shift data
	}
}


void perform_batched_DNEWS_iterations_S5_S6_no_sequential_DNEWS(int remaining_iterations)
{
	while(remaining_iterations > 0)
	{
		if(remaining_iterations % 8 == 0)//Check if remaining shift is divisible by 8
		{
			scamp5_kernel_begin();//Kernel to shift data 8 times
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
			scamp5_kernel_end();
			remaining_iterations -= 8;//Update remaining shift
		}
		else
		{
			if(remaining_iterations % 4 == 0)//Check if remaining shift is divisible by 4
			{
				scamp5_kernel_begin();//Kernel to shift data 4 times
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				DNEWS0(S6,S5);
				MOV(S5,S6);
				scamp5_kernel_end();
				remaining_iterations -= 4;//Update remaining shift
			}
			else
			{
				if(remaining_iterations % 2 == 0)//Check if remaining shift is divisible by 2
				{
					scamp5_kernel_begin();//Kernel to shift data 2 times
					DNEWS0(S6,S5);
					MOV(S5,S6);
					DNEWS0(S6,S5);
					MOV(S5,S6);
					scamp5_kernel_end();
					remaining_iterations -= 2;//Update remaining shift
				}
				else
				{
					scamp5_kernel_begin();//Kernel to shift data once
						DNEWS0(S6,S5);
						MOV(S5,S6);
					scamp5_kernel_end();
					remaining_iterations -= 1;//Update remaining shift
				}
			}
		}
	}
}

void batched_shift_S5_no_decay(int shiftx, int shifty)
{
	if(shiftx > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RE);
			SET(RW);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6_no_sequential_DNEWS(shiftx);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RW);
			SET(RE);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6_no_sequential_DNEWS(-shiftx);//Call function to perform DNEWS instructions and shift data
	}

	if(shifty > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RS,RE,RW);
			SET(RN);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6_no_sequential_DNEWS(shifty);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RE,RW);
			SET(RS);
		scamp5_kernel_end();
		perform_batched_DNEWS_iterations_S5_S6_no_sequential_DNEWS(-shifty);//Call function to perform DNEWS instructions and shift data
	}
}


void perform_DNEWS_iterations_S5_S6(int iterations)
{
	//Perform DNEWS X times by calling a simple kernel X times repeatedly
	//This should be the least efficient way to do this...
	for(int i = 0 ; i < iterations ; i++)
	{
		//Perform a single DNEWS instruction, then copy the result back with MOV
		scamp5_kernel_begin();
			DNEWS0(S6,S5);
			MOV(S5,S6);
		scamp5_kernel_end();
	}
}

void simple_shift_S5(int shiftx, int shifty)
{
	if(shiftx > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RE);
			SET(RW);
		scamp5_kernel_end();
		perform_DNEWS_iterations_S5_S6(shiftx);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RS,RW);
			SET(RE);
		scamp5_kernel_end();
		perform_DNEWS_iterations_S5_S6(-shiftx);//Call function to perform DNEWS instructions and shift data
	}

	if(shifty > 0)
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RS,RE,RW);
			SET(RN);
		scamp5_kernel_end();
		perform_DNEWS_iterations_S5_S6(shifty);//Call function to perform DNEWS instructions and shift data
	}
	else
	{
		scamp5_kernel_begin();//Setup DNEWS direction control registers
			CLR(RN,RE,RW);
			SET(RS);
		scamp5_kernel_end();
		perform_DNEWS_iterations_S5_S6(-shifty);//Call function to perform DNEWS instructions and shift data
	}
}
