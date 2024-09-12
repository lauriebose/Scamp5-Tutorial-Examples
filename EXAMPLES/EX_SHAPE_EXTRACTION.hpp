#include <scamp5.hpp>
#include <random>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("S0",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("S1 Flooding Source",0,display_size,display_size);
    vs_handle display_02 = vs_gui_add_display("S2 = S1 flooding, masked by S0",0,display_size*2,display_size);

	bool generate_boxes = true;
	vs_handle gui_button_generate_boxes = vs_gui_add_button("generate boxes");
	vs_on_gui_update(gui_button_generate_boxes,[&](int32_t new_value)//function that will be called whenever button is pressed
	{
		generate_boxes = true;//trigger generation of boxes
    });

	bool extract_a_shape_trigger = false;
	vs_handle gui_button_extract_shape = vs_gui_add_button("extract a shape");
	vs_on_gui_update(gui_button_extract_shape,[&](int32_t new_value)//function that will be called whenever button is pressed
	{
		extract_a_shape_trigger = true;
    });

	bool eliminate_shape_trigger = false;
	vs_handle gui_button_eliminate_shape = vs_gui_add_button("eliminate extracted shape");
	vs_on_gui_update(gui_button_eliminate_shape,[&](int32_t new_value)//function that will be called whenever button is pressed
	{
		eliminate_shape_trigger = true;
    });

	vs_handle gui_button_extract_and_eliminate_shape = vs_gui_add_button("extract+eliminate a shape");
	vs_on_gui_update(gui_button_extract_and_eliminate_shape,[&](int32_t new_value)//function that will be called whenever button is pressed
	{
		extract_a_shape_trigger = true;
		eliminate_shape_trigger = true;
    });

	//Setup objects for random number generation
	//This code looks like nonsense as the std classes here overload the function call operator...
	std::random_device rd;//A true random number
	std::mt19937 gen(rd()); // Mersenne Twister pseudo random number generator, seeded with a true random number
	int random_min_value = 0;
	int random_max_value = 255;
	//Create distribution object that can be used to map a given random value to a value of the distribution
	std::uniform_int_distribution<> distr(random_min_value, random_max_value);


    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATES RANDOM BOXES IN DREG S0 IF BUTTON WAS PRESSED (OR PROGRAM JUST STARTED)

			if(generate_boxes)
			{
				const int boxes_to_add = 50;
				const int boxes_to_subtract = 25;

				//Generate a set of random rectangles across DREG plane S0
				{
					scamp5_kernel_begin();
						CLR(S0); //Clear content of S0
					scamp5_kernel_end();
					for(int n = 0 ; n < boxes_to_add ; n++)
					{
						//Load box of random location and dimensions into S5
						int pos_x = distr(gen);
						int pos_y = distr(gen);
						int width = 1+distr(gen)/5;
						int height = 1+distr(gen)/5;
						DREG_load_centered_rect(S5,pos_x,pos_y,width,height);
						scamp5_kernel_begin();
							OR(S0,S5);//Add box in S5 to content of S0
						scamp5_kernel_end();
					}
				}

				//Generate another set of random rectangles across DREG plane S1
				scamp5_kernel_begin();
					CLR(S6); //Clear content of S1
				scamp5_kernel_end();
				for(int n = 0 ; n < boxes_to_subtract ; n++)
				{
					//Load box of random location and dimensions into S5
					int pos_x = distr(gen);
					int pos_y = distr(gen);
					int width = 1+distr(gen)/5;
					int height = 1+distr(gen)/5;
					DREG_load_centered_rect(S5,pos_x,pos_y,width,height);
					scamp5_kernel_begin();
						OR(S6,S5);//Add box in S5 to content of S0
					scamp5_kernel_end();
				}

				//Subtract the rectangles in S6 from those of S2
				scamp5_kernel_begin();
					NOT(S5,S6);//S5 = Inverted content of S6
					AND(S0,S5,S0);//perform AND to "Subtract" S1 rectangles from S0
				scamp5_kernel_end();

				generate_boxes = false;
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//PERFORM FLOODING OF S0, ORGINATING FROM A SPECIFIC POINT

			if(extract_a_shape_trigger)
			{
				extract_a_shape_trigger = false;//Reset Trigger

				uint8_t event_data_array [2];
				//Scan Events from S5 register plane
				//Examines the S5 register of each PE, returning the locations of PEs in which S5 == 1
				//The locations of these PEs will be written to the array "event_data_array" as (x,y) coordinate pairs
				//Once all events have been scanned will write "(0,0)" coordinates into the array
				scamp5_scan_events(S0,event_data_array,1);

				int event_xpos = event_data_array[0];
				int event_ypos = event_data_array[1];
				scamp5_load_point(S1,event_ypos,event_xpos);

				//Perform Flooding using native instructions
				{
					scamp5_kernel_begin();
						SET (RN,RS,RE,RW);//Set all DNEWS register so flooding is performed in all directions across whole processor array

						SET(RF);//Reset digital Flag = 1 across whole processor array, as operations targeting RZ are Flagged by RF
						MOV(RP,S1);//Copy loaded point into RZ
						MOV(RF,S0);//Copy the content of S0 into RF to act as the Mask during flooding
					scamp5_kernel_end();

					//Perform flooding iterations
					int flood_iterations = 10;
					for(int n = 0 ; n < flood_iterations ; n++)
					{
						scamp5_kernel_begin();
							PROP0();
						scamp5_kernel_end();
					}

					scamp5_kernel_begin();
						MOV(S2,RP);//Copy result of flooding S2
					scamp5_kernel_end();
				}
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//PERFORM FLOODING OF S0, ORGINATING FROM A SPECIFIC POINT

			if(eliminate_shape_trigger)
			{
				eliminate_shape_trigger = false;
				scamp5_kernel_begin();
					NOT(S6,S2);//Invert DREG plane holding extracted shape and store in S6
					AND(S0,S0,S6);//Eliminate extracted shape from S0 by performing AND operation with S6
				scamp5_kernel_end();
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT THE BOUNDING BOX OF THE FLOODED SHAPE


			uint8_t bb_data [4];
			scamp5_output_boundingbox(S2,display_00,bb_data);
			scamp5_display_boundingbox(display_01,bb_data,1);
			scamp5_display_boundingbox(display_02,bb_data,1);

			int bb_top = bb_data[0];
			int bb_bottom = bb_data[2];
			int bb_left = bb_data[1];
			int bb_right = bb_data[3];

			int bb_width = bb_right-bb_left;
			int bb_height = bb_bottom-bb_top;
			int bb_center_x = (bb_left+bb_right)/2;
			int bb_center_y = (bb_top+bb_bottom)/2;

			vs_post_text("bounding box data X:%d Y:%d W:%d H:%d\n",bb_center_x,bb_center_y,bb_width,bb_height);


	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//REFRESH CONTENT OF S0,S1,S2 TO PREVENT IT DECAYING
			scamp5_kernel_begin();
				REFRESH(S0);
				REFRESH(S1);
				REFRESH(S2);
			scamp5_kernel_end();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			scamp5_output_image(S0,display_00);
			scamp5_output_image(S1,display_01);
			scamp5_output_image(S2,display_02);
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

