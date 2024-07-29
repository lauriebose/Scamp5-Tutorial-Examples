#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

#include <iostream>
#include <string>

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch event_readout_timer;

void DREG_load_centered_rect(dreg_t dr, int centre_x, int centre_y, int width, int height);

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int display_size = 2;
		auto display_00 = vs_gui_add_display("S5",0,0,display_size);
		auto display_01 = vs_gui_add_display("Scanned Events From S5",0,display_size,display_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		const int max_events_to_scan = 10;
	    int event_to_scan = 3;
	    vs_gui_add_slider("event_to_scan: ",0,max_events_to_scan,event_to_scan,&event_to_scan);
	    uint8_t event_data [max_events_to_scan*2];

	    int point_x = 120;
	    int point_y = 40;
	    vs_gui_add_slider("point_x: ",0,255,point_x,&point_x);
	    vs_gui_add_slider("point_y: ",0,255,point_y,&point_y);

	    int point_x2 = 120;
	    int point_y2 = 120;
	    vs_gui_add_slider("point_x2: ",0,255,point_x2,&point_x2);
	    vs_gui_add_slider("point_y2: ",0,255,point_y2,&point_y2);

	    int draw_event_ids = 1;
	    vs_gui_add_switch("draw event ids",draw_event_ids == 1,&draw_event_ids);

    //CONTINOUS FRAME LOOP
    while(true)
    {

        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //LOAD EXAMPLE DATA TO DREG TO SCAN EVENTS FROM


        scamp5_load_pattern(S5,70,70,128,128);
        scamp5_load_point(RN,point_y,point_x);
        scamp5_load_point(RS,point_y2,point_x2);
        scamp5_kernel_begin();
			OR(S5,RN);
			OR(S5,RS);
		scamp5_kernel_end();

		event_readout_timer.reset();
		scamp5_scan_events(S5,event_data,event_to_scan);
		int time_spent_on_event_readout = event_readout_timer.get_usec();

//		scamp5_output_events(S5,display_00,event_to_scan);


		 scamp5_kernel_begin();
			CLR(S4);
		scamp5_kernel_end();

		std::string event_data_string;
		for(int n = 0 ; n < event_to_scan ; n++)
		{
			int event_x = event_data[n*2];
			int event_y = event_data[n*2 + 1];

			if(event_x == 0 && event_y == 0)
			{
				break;
			}

			//DRAW SCANNED EVENTS ON OVERLAY DISPLAY
			scamp5_load_point(S6,event_y,event_x);
			scamp5_kernel_begin();
				OR(S4,S6);
			scamp5_kernel_end();

			//PRINT EVENT XYs
			event_data_string += "(" + std::to_string(event_x) + "," + std::to_string(event_y) + "), ";

			//DRAW INDEX OF SCANNED EVENTS ON DISPLAY
			if(draw_event_ids)
			{
				std::string str = std::to_string(n);
				vs_gui_display_text(display_01,event_x,event_y,str.c_str());
			}

		}
		vs_post_text("%s\n",event_data_string.c_str());

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

			output_timer.reset();
		    scamp5_output_image(S5,display_00);
	        scamp5_output_image(S4,display_01);
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

	scamp5_load_rect(dr, top_left_row, top_left_column, top_left_row+height-1, top_left_column+width-1);
}
