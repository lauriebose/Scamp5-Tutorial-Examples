#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

#include <iostream>
#include <string>

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch event_readout_timer;



int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int display_size = 3;
		auto display_00 = vs_gui_add_display("S5",0,0,display_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		const int max_events_to_scan = 100;
		uint8_t event_data_array [max_events_to_scan*2];

	    int number_of_events_to_scan = 3;
	    vs_gui_add_slider("event_to_scan: ",0,max_events_to_scan,number_of_events_to_scan,&number_of_events_to_scan);

	    //Sliders to control location of white pixel to use as example of event readout
	    int point_x = 120;
	    int point_y = 40;
	    vs_gui_add_slider("point_x: ",0,255,point_x,&point_x);
	    vs_gui_add_slider("point_y: ",0,255,point_y,&point_y);

	    //Sliders to adjust the grid of white pixels to use as example of event readout
	    int point_grid_offx = 70;
	    int point_grid_offy = 60;
	    vs_gui_add_slider("point_grid_offx: ",0,255,point_grid_offx,&point_grid_offx);
	    vs_gui_add_slider("point_grid_offy: ",0,255,point_grid_offy,&point_grid_offy);
	    int point_grid_spacing_x = 80;
	    int point_grid_spacing_y = 80;
	    vs_gui_add_slider("point_grid_spacing_x: ",1,255,point_grid_spacing_x,&point_grid_spacing_x);
	    vs_gui_add_slider("point_grid_spacing_y: ",1,255,point_grid_spacing_y,&point_grid_spacing_y);

	    int draw_event_indexes = 1;
	    vs_gui_add_switch("draw event indexes",draw_event_indexes == 1,&draw_event_indexes);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

       //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //LOAD EXAMPLE DATA TO DREG S5 TO USE IN DEMONSTRATING EVENT READOUT

			scamp5_kernel_begin();
				CLR(S5);
			scamp5_kernel_end();

			//Generate grid of 1s/White Pixels
			{
				//Loop through x,y grid location
				int start_x = point_grid_offx%point_grid_spacing_x;
				int start_y = point_grid_offy%point_grid_spacing_y;
				for(int x= start_x ; x < 256; x+=point_grid_spacing_x)
				{
					for(int y = start_y ; y < 256; y+=point_grid_spacing_y)
					{
						 //Generate a point/white pixel image
						 //The RN register will == 1 at the PE at location x,y, RN in all other PEs == 0
						 scamp5_load_point(RN,x,y);

						 scamp5_kernel_begin();
						 	//Combine this new point with the content of S5
							OR(S5,RN);
						 scamp5_kernel_end();
					}
				}
			}

			//Generate single 1/White Pixel
			scamp5_load_point(RN,point_y,point_x);
			scamp5_kernel_begin();
			    //Combine this new point with the content of S5
				OR(S5,RN);
			scamp5_kernel_end();


	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//DEMONSTRATE EVENT READOUT ON S5

			event_readout_timer.reset();

				//Scan Events from S5 register plane
				//Examines the S5 register of each PE, returning the locations of PEs in which S5 == 1
				//The locations of these PEs will be written to the array "event_data_array" as (x,y) coordinate pairs
				//Once all events have been scanned will write "(0,0)" coordinates into the array
				scamp5_scan_events(S5,event_data_array,number_of_events_to_scan);

			int time_spent_on_event_readout = event_readout_timer.get_usec();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//DISPLAY EVENT INFORMATION

			//Show the Events scanned, will draw little green boxes around each location in "event_data_array"
			scamp5_display_events(display_00,event_data_array,number_of_events_to_scan);

			//Draw indexes of scanned events next to their location
			if(draw_event_indexes)
			{
				//Loop through events, draw the index of each event
				bool prev_event_was_00 = false;
				for(int n = 0 ; n < number_of_events_to_scan ; n++)
				{
					int event_x = event_data_array[n*2];
					int event_y = event_data_array[n*2 + 1];

					bool event_is_00 = (event_x == 0 && event_y == 0) ? true : false;
					if(event_is_00 && prev_event_was_00)
					{
						//If two (0,0)s in a row then must have reached end of Event data so can just exit loop
						break;
					}
					prev_event_was_00 = event_is_00;

					//Draw index as text on a display
					const std::array<uint8_t, 4> text_color = {127, 255, 127, 255};
					std::string tmp_str = std::to_string(n);
					vs_gui_display_text(display_00, event_x, event_y, tmp_str.c_str(), text_color);
				}
    		}

			//Draw event locations to text output
			std::string event_data_string;
			for(int n = 0 ; n < number_of_events_to_scan ; n++)
			{
				int event_x = event_data_array[n*2];
				int event_y = event_data_array[n*2 + 1];

				//Add event data to string
				event_data_string += "(" + std::to_string(event_x) + "," + std::to_string(event_y) + "), ";
				if(n%8 == 0 && n != 0)
				{
					event_data_string += "\n";
				}
			}
			vs_post_text("%s\n",event_data_string.c_str());

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

			output_timer.reset();
		    scamp5_output_image(S5,display_00);
	    	int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("event read out time %d, frame time %d microseconds(%%%d image output), potential FPS ~%d \n",time_spent_on_event_readout,frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}
