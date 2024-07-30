#include <scamp5.hpp>
using namespace SCAMP5_PE;

void DREG_load_centered_rect(dreg_t reg, int x, int y, int width, int height);

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("S0 (box)",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("S1 (box2)",0,display_size,display_size);
    vs_handle display_10 = vs_gui_add_display("S3 = S0 AND S1",display_size,0,display_size);
    vs_handle display_11 = vs_gui_add_display("GLOBAL OR S3",display_size,display_size,display_size);

    int box_x, box_y, box_width, box_height;
    vs_gui_add_slider("box x: ", 0, 255, 128, &box_x);
    vs_gui_add_slider("box y: ", 0, 255, 128, &box_y);
    vs_gui_add_slider("box width: ", 0, 128, 32, &box_width);
    vs_gui_add_slider("box height: ", 0, 128, 64, &box_height);

    int point_x, point_y;
    vs_gui_add_slider("point_x: ", 0, 255, 128, &point_x);
    vs_gui_add_slider("point_y: ", 0, 255, 96, &point_y);

    int point2_x, point2_y;
     vs_gui_add_slider("point2_x: ", 0, 255, 72, &point2_x);
     vs_gui_add_slider("point2_y: ", 0, 255, 55, &point2_y);

    while(1)
    {
    	frame_timer.reset();



    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      	//LOAD CONTENT INTO DREGS S0 AND S1

			//Load rect into S0
			DREG_load_centered_rect(S0,box_x,box_y,box_width,box_height);

			//Load two points into S1
			scamp5_load_point(S5,point_y,point_x);
			scamp5_load_point(S6,point2_y,point2_x);
			scamp5_kernel_begin();
				MOV(S1,S5);
				OR(S1,S6);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE VARIOUS LOGIC OPERATIONS
			scamp5_kernel_begin();
				AND(S3,S0,S1);
			scamp5_kernel_end();

			bool point_inside_rect = scamp5_global_or(S3,0,0,255,255) > 0 ? true : false;

			if(point_inside_rect)
			{
				vs_post_text("TRUE \n");
			   scamp5_kernel_begin();
					SET(S2);
				scamp5_kernel_end();
			}
			else
			{
				vs_post_text("FALSE \n");
				scamp5_kernel_begin();
					CLR(S2);
				scamp5_kernel_end();
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			 //show DREG of box & box2
			scamp5_output_image(S0,display_00);
			scamp5_output_image(S1,display_01);
			scamp5_output_image(S3,display_10);//show OR result
			scamp5_output_image(S2,display_11);//show AND result
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

//		vs_post_text("box(%d,%d,%d,%d) box2(%d,%d,%d,%d)\n",box_x,box_y,box_width,box_height,box2_x,box2_y,box2_width,box2_height);

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

