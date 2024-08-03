#include <scamp5.hpp>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;


vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 1;
    vs_handle display_00 = vs_gui_add_display("S0 (box1)",0,0,2);
    vs_handle display_01 = vs_gui_add_display("S1 (box2)",0,2,2);
    vs_handle display_10 = vs_gui_add_display("S0 OR S1",2,0,display_size);
    vs_handle display_11 = vs_gui_add_display("S0 AND S1",2,display_size,display_size);
    vs_handle display_12 = vs_gui_add_display("NOT (S0 AND S1)",2,display_size*2,display_size);
    vs_handle display_13 = vs_gui_add_display("S0 XOR S1",2,display_size*3,display_size);

    int box1_x, box1_y, box1_width, box1_height;
    vs_gui_add_slider("box1 x: ", 0, 255, 128, &box1_x);
    vs_gui_add_slider("box1 y: ", 0, 255, 128, &box1_y);
    vs_gui_add_slider("box1 width: ", 0, 128, 32, &box1_width);
    vs_gui_add_slider("box1 height: ", 0, 128, 64, &box1_height);

    int box2_x, box2_y, box2_width, box2_height;
    vs_gui_add_slider("box2 x: ", 0, 255, 96, &box2_x);
    vs_gui_add_slider("box2 y: ", 0, 255, 96, &box2_y);
    vs_gui_add_slider("box2 width: ", 0, 128, 64, &box2_width);
    vs_gui_add_slider("box2 height: ", 0, 128, 64, &box2_height);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //load box1 & box2 into DREGs
        DREG_load_centered_rect(S0,box1_x,box1_y,box1_width,box1_height);
        DREG_load_centered_rect(S1,box2_x,box2_y,box2_width,box2_height);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE VARIOUS LOGIC OPERATIONS
        scamp5_kernel_begin();
        	OR(S6,S0,S1);//S6 = box1 OR box2

        	AND(S5,S0,S1);//S5 = box1 AND box2

        	NOT(S4,S5);//S4 = NOT(box1 AND box2)

        	AND(S3,S6,S4); //S3 = S6 AND S4 == (box1 OR box2) AND NOT(box1 and box1) == box1 XOR box2
        scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			 //show DREG of box1 & box2
			scamp5_output_image(S0,display_00);
			scamp5_output_image(S1,display_01);
			scamp5_output_image(S6,display_10);//show OR result
			scamp5_output_image(S5,display_11);//show AND result
			scamp5_output_image(S4,display_12);//show NOT(AND) result
			scamp5_output_image(S3,display_13);//show XOR result

			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

		vs_post_text("box1(%d,%d,%d,%d) box2(%d,%d,%d,%d)\n",box1_x,box1_y,box1_width,box1_height,box2_x,box2_y,box2_width,box2_height);

  		int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
  		int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
  		int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
        vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}
