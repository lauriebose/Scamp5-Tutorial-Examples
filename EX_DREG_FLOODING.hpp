#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch flooding_timer;
vs_stopwatch drawing_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

        const int disp_size = 2;
	    auto display_00 = vs_gui_add_display("Flooding source",0,0,disp_size);
	    auto display_01 = vs_gui_add_display("Flooding mask",0,disp_size,disp_size);
	    auto display_02 = vs_gui_add_display("Source after flooding",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    //control the box drawn into DREG & used as the flooding source
	    int box_posx, box_posy,box_width,box_height;
		vs_gui_add_slider("box_posx: ",1,256,64,&box_posx);
		vs_gui_add_slider("box_posy: ",1,256,125,&box_posy);
		vs_gui_add_slider("box_width: ",1,100,20,&box_width);
		vs_gui_add_slider("box_height: ",1,100,20,&box_height);

		//The number of iterations performed, determines the extent/distance flooding can reach
	    int flood_iterations = 10;
	    vs_gui_add_slider("flood_iterations", 0,20,flood_iterations,&flood_iterations);

	    //Select if PEs on the edge of the array/image act as sources during flooding
		int flood_from_boundaries = 0;
		vs_gui_add_switch("flood_from_boundaries",true,&flood_from_boundaries);

		//Add switches for selecting the content of the RN,RS,RE,RW DREG, which control the directions of flooding
		int set_RN = 1;
		vs_gui_add_switch("set_RN",set_RN == 1,&set_RN);
		int set_RS = 1;
		vs_gui_add_switch("set_RS",set_RS == 1,&set_RS);
		int set_RE = 1;
		vs_gui_add_switch("set_RE",set_RE == 1,&set_RE);
		int set_RW = 1;
		vs_gui_add_switch("set_RW",set_RW == 1,&set_RW);


    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //DRAW CONTENT INTO DIGITAL REGISTERS FOR FLOODING EXAMPLE

        	drawing_timer.reset();

			//DRAW CONTENT INTO THE FLOODING SOURCE REGISTER, FLOODING STARTS FROM THE WHITE PIXEL OF THIS REGISTER
			{
    			scamp5_kernel_begin();
    				CLR(S1);
    			scamp5_kernel_end();
				scamp5_draw_begin(S1);
					scamp5_draw_rect(box_posy,box_posx,box_height+box_posy,box_width+box_posx);
				scamp5_draw_end();
				scamp5_kernel_begin();
					MOV(S3,S1);
				scamp5_kernel_end();
			}


			//DRAW CONTENT INTO THE FLOODING MASK REGISTER, FLOODING IS RESTRICTED TO THE WHITE PIXEL OF THIS REGISTER
			{
				scamp5_kernel_begin();
					CLR(S2);
				scamp5_kernel_end();

				scamp5_draw_begin(S2);
					scamp5_draw_circle(127,127,100);
					scamp5_draw_circle(127,127,50);
					scamp5_draw_line(10,0,10,200);
					scamp5_draw_line(10,200,100,200);
				scamp5_draw_end();
			}

			int time_spent_drawing = drawing_timer.get_usec();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SET DREG WHICH CONTROL THE DIRECTIONS IN WHICH FLOODING MAY OCCUR

			scamp5_kernel_begin();
				CLR(RN,RS,RE,RW);
			scamp5_kernel_end();

			if(set_RN)
			{
				scamp5_kernel_begin();
					SET(RN);
				scamp5_kernel_end();
			}
			if(set_RS)
			{
				scamp5_kernel_begin();
					SET(RS);
				scamp5_kernel_end();
			}
			if(set_RE)
			{
				scamp5_kernel_begin();
					SET(RE);
				scamp5_kernel_end();
			}
			if(set_RW)
			{
				scamp5_kernel_begin();
					SET(RW);
				scamp5_kernel_end();
			}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//shift content of S1 using DNEWS

			//PERFORM FLOODING
			flooding_timer.reset();

			//FLOODING USING API
			scamp5_flood(S1,S2,flood_from_boundaries,flood_iterations,true);

			int time_spent_flooding = flooding_timer.get_usec();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();

			scamp5_output_image(S3,display_00);
			scamp5_output_image(S2,display_01);
			scamp5_output_image(S1,display_02);
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("time spent drawing %d \n",time_spent_drawing);
			vs_post_text("time spent flooding %d \n",time_spent_flooding);
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}
