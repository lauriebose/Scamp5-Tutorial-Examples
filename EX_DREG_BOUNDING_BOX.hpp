#include <scamp5.hpp>
#include <random>
using namespace SCAMP5_PE;

void DREG_load_centered_rect(dreg_t reg, int x, int y, int width, int height);

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 3;
    vs_handle display_00 = vs_gui_add_display("S0",0,0,display_size);

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


        //load box & box2 into DREGs
        DREG_load_centered_rect(S4,box_x,box_y,box_width,box_height);
        DREG_load_centered_rect(S5,box2_x,box2_y,box2_width,box2_height);
        DREG_load_centered_rect(S6,box3_x,box3_y,box3_width,box3_height);
    	scamp5_kernel_begin();
			MOV(S0,S4);
			OR(S0,S5);
			OR(S0,S6);
		scamp5_kernel_end();


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//GET BOUNDING BOX OF S0's CONTENT

			uint8_t bb_data [4];
			scamp5_output_boundingbox(S0,display_00,bb_data);

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

