#include <scamp5.hpp>
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 3;
    vs_handle display_00 = vs_gui_add_display("S0 (box1)",0,0,display_size);

    int r,c,rx,cx;
    vs_gui_add_slider("r: ",0,255,127,&r);
    vs_gui_add_slider("c: ",0,255,127,&c);
    vs_gui_add_slider("rx: ",0,255,10,&rx);
    vs_gui_add_slider("cx: ",0,255,10,&cx);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE VARIOUS LOGIC OPERATIONS
        scamp5_load_pattern(S1,r,c,rx,cx);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			 //show DREG of box1 & box2
			scamp5_output_image(S1,display_00);

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
