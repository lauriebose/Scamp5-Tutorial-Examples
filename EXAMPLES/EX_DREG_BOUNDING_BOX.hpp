#include <scamp5.hpp>
#include "MISC/MISC_FUNCS.hpp"
#include <string>
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 3;
    vs_handle display_00 = vs_gui_add_display("S0",0,0,display_size);

    //Setup sliders to control boxes used to demonstrate bounding box readout
    int box_x, box_y, box_width, box_height;
    vs_gui_add_slider("box x: ", 0, 255, 128, &box_x);
    vs_gui_add_slider("box y: ", 0, 255, 128, &box_y);
    vs_gui_add_slider("box width: ", 0, 128, 32, &box_width);
    vs_gui_add_slider("box height: ", 0, 128, 64, &box_height);

    int box2_x, box2_y, box2_width, box2_height;
    vs_gui_add_slider("box2 x: ", 0, 255, 96, &box2_x);
    vs_gui_add_slider("box2 y: ", 0, 255, 96, &box2_y);
    vs_gui_add_slider("box2 width: ", 0, 128, 64, &box2_width);
    vs_gui_add_slider("box2 height: ", 0, 128, 64, &box2_height);

    int box3_x, box3_y, box3_width, box3_height;
    vs_gui_add_slider("box3 x: ", 0, 255, 96, &box3_x);
    vs_gui_add_slider("box3 y: ", 0, 255, 96, &box3_y);
    vs_gui_add_slider("box3 width: ", 0, 128, 64, &box3_width);
    vs_gui_add_slider("box3 height: ", 0, 128, 64, &box3_height);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	//LOAD CONTENT INTO A DIGITAL REGISTER PLANE S0 FOR DEMONSTRATING BOUNDING BOX OUTPUT

			//Load boxes into DREG planes
			DREG_load_centered_rect(S4,box_x,box_y,box_width,box_height);
			DREG_load_centered_rect(S5,box2_x,box2_y,box2_width,box2_height);
			DREG_load_centered_rect(S6,box3_x,box3_y,box3_width,box3_height);

			//Combine boxes into single DREG plane
			scamp5_kernel_begin();
				MOV(S0,S4);
				OR(S0,S5);
				OR(S0,S6);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//GET BOUNDING BOX OF S0's CONTENT

			uint8_t bb_data [4];
			//This function get bounding box of digital register plane's (S0's) content
			//That is the bounding box of the 1s/white pixels present in S0
			//Will fill the passed array "bb_data" with the bounding boxes' top-left and bottom-right coordinates
			scamp5_scan_boundingbox(S0,bb_data);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//DISPLAY THE RETRIEVED BOUNDING BOX

			//Will draw a bounding box to a display based on an array of coordinates, here the coordinates we just retrieved
			scamp5_display_boundingbox(display_00,bb_data,1);

			int bb_top = bb_data[0];
			int bb_bottom = bb_data[2];
			int bb_right = bb_data[1];
			int bb_left = bb_data[3];

			//Calculate the center and dimensions of the bounding box
			int bb_width = bb_left-bb_right;
			int bb_height = bb_bottom-bb_top;
			int bb_center_x = (bb_right+bb_left)/2;
			int bb_center_y = (bb_top+bb_bottom)/2;

			//Draw bounding boxes' location and dimensions to the display, next to the bounding box itself
			const std::array<uint8_t, 4> text_color = {127, 255, 127, 255};
			std::string tmp_str = "X:" + std::to_string(bb_center_x) + " Y:" + std::to_string(bb_center_y) + " Width:" +  std::to_string(bb_width) + " Height:" +  std::to_string(bb_height);
			vs_gui_display_text(display_00, bb_left, bb_top, tmp_str.c_str(), text_color);

			vs_post_text("bounding box data X:%d Y:%d W:%d H:%d\n",bb_center_x,bb_center_y,bb_width,bb_height);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			scamp5_output_image(S0,display_00);
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
