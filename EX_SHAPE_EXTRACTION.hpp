#include <scamp5.hpp>
#include <random>
using namespace SCAMP5_PE;

void DREG_load_centered_rect(dreg_t reg, int x, int y, int width, int height);

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("S0",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("S1 Flooding Source",0,display_size,display_size);
    vs_handle display_02 = vs_gui_add_display("S2 = S1 flooding, masked by S0",0,display_size*2,display_size);

    //The point from which flooding will start from
    int flood_source_x, flood_source_y;
	vs_gui_add_slider("flood_source_x: ",1,256,128,&flood_source_x);
	vs_gui_add_slider("flood_source_y: ",1,256,128,&flood_source_y);

	//The number of iterations performed, determines the extent/distance flooding can reach
    int flood_iterations = 10;
    vs_gui_add_slider("flood_iterations", 0,20,flood_iterations,&flood_iterations);

	bool generate_boxes = true;
	vs_handle gui_button_generate_boxes = vs_gui_add_button("generate_boxes");
	//function that will be called whenever button is pressed
	vs_on_gui_update(gui_button_generate_boxes,[&](int32_t new_value)
	{
		generate_boxes = true;//trigger generation of boxes
    });

	int output_bounding_box = 1;
	vs_gui_add_switch("output_bounding_box", true, &output_bounding_box);

	//Toggle if to refresh the content of S0 each frame to prevent it decaying
    int refresh_S0 = 1;
    vs_gui_add_switch("refresh_S0", true, &refresh_S0);

    //Objects for random number generation
		std::random_device rd;
		std::mt19937 gen(rd()); // Mersenne Twister generator
		int min = 0;
		int max = 255;
		std::uniform_int_distribution<> distr(min, max);//Create distribution to sample from

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATES RANDOM BOXES IN DREG S0 IF BUTTON WAS PRESSED (OR PROGRAM JUST STARTED)

			if(generate_boxes)
			{
				scamp5_kernel_begin();
					CLR(S0); //Clear content of S0
				scamp5_kernel_end();
				for(int n = 0 ; n < 50 ; n++)
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


				scamp5_kernel_begin();
					CLR(S1); //Clear content of S0
				scamp5_kernel_end();
				for(int n = 0 ; n < 25 ; n++)
				{
					//Load box of random location and dimensions into S5
					int pos_x = distr(gen);
					int pos_y = distr(gen);
					int width = 1+distr(gen)/5;
					int height = 1+distr(gen)/5;
					DREG_load_centered_rect(S5,pos_x,pos_y,width,height);

					scamp5_kernel_begin();
						OR(S1,S5);//Add box in S5 to content of S0
					scamp5_kernel_end();
				}

				scamp5_kernel_begin();
					NOT(S5,S1);
					AND(S0,S5,S0);
				scamp5_kernel_end();

				generate_boxes = false;
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM FLOODING OF S0, ORGINATING FROM A SPECIFIC POINT

			//Load specified point into S1
			scamp5_load_point(S1,flood_source_y,flood_source_x);
			//Perform Flooding using native instructions
			{
				scamp5_kernel_begin();
					SET (RN,RS,RE,RW);//Set all DNEWS register so flooding is performed in all directions across whole processor array
					MOV(S2,S1);
				scamp5_kernel_end();

				//Perform flooding iterations
				scamp5_flood(S2,S0,0,flood_iterations);
			}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OPTIONALLY OUTPUT THE BOUNDING BOX OF THE FLOODED SHAPE

			if(output_bounding_box)
			{
			    uint8_t bb_data [4];
				scamp5_output_boundingbox(S2,display_02,bb_data);

				int bb_top = bb_data[0];
				int bb_bottom = bb_data[2];
				int bb_left = bb_data[1];
				int bb_right = bb_data[3];

				int bb_width = bb_right-bb_left;
				int bb_height = bb_bottom-bb_top;
				int bb_center_x = (bb_left+bb_right)/2;
				int bb_center_y = (bb_top+bb_bottom)/2;

				vs_post_text("bounding box data X:%d Y:%d W:%d H:%d\n",bb_center_x,bb_center_y,bb_width,bb_height);
			}

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OPTIONALLY REFRESH CONTENT OF S0 TO PREVENT IT DECAYING

			if(refresh_S0)
			{
				scamp5_kernel_begin();
					REFRESH(S0);
				scamp5_kernel_end();
			}

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

