#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

const areg_t AREG_image = A;
const areg_t AREG_vertical_edges = B;
const areg_t AREG_horizontal_edges = C;

const dreg_t DREG_combined_edges = S0;
const dreg_t DREG_vertical_edges = S1;
const dreg_t DREG_horizontal_edges = S2;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

    int disp_size = 2;
    auto display_00 = vs_gui_add_display("Captured Image",0,0,disp_size);
    auto display_01 = vs_gui_add_display("Vertical Edges",0,disp_size,disp_size);
    auto display_02 = vs_gui_add_display("Horizontal Edges",0,disp_size*2,disp_size);

    auto display_10 = vs_gui_add_display("Combined Edges",disp_size,0,disp_size);
    auto display_11 = vs_gui_add_display("Vertical Edges Thresholded",disp_size,disp_size,disp_size);
    auto display_12 = vs_gui_add_display("Horizontal Edges Thresholded",disp_size,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

    int edge_threshold = 32;
    vs_gui_add_slider("edge_threshold x",0,127,edge_threshold,&edge_threshold);

    int output_only_combined_edges = 0;
    vs_gui_add_switch("output_only_combined_edges",0,&output_only_combined_edges);

    // Frame Loop
    while(1)
    {
        frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	scamp5_kernel_begin();
    		get_image(A,E);
    	scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE VERTICAL EDGE IMAGE

			scamp5_kernel_begin();
				//COPY IMAGE TO F AND SHIFT ONE "PIXEL" RIGHT
				bus(NEWS,AREG_image);
				bus(F,XW);

				//SAVE THE ABSOLUTE DIFFERENCE BETWEEN THE SHIFTED IMAGE AND ORIGINAL AS THE VERTICAL EDGES RESULT
				sub(F,F,AREG_image);
				abs(AREG_vertical_edges,F);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //COMPUTE HORIZONTAL EDGE IMAGE

			scamp5_kernel_begin();
				//COPY IMAGE TO F AND SHIFT ONE "PIXEL" UP
				bus(NEWS,AREG_image);
				bus(F,XS);

				//SAVE THE ABSOLUTE DIFFERENCE BETWEEN THE SHIFTED IMAGE AND ORIGINAL AS THE HORIZONTAL EDGES RESULT
				sub(F,F,AREG_image);
				abs(AREG_horizontal_edges,F);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //THRESHOLD AREG EDGE DATA TO BINARY


			scamp5_in(F,edge_threshold);//LOAD THRESHOLD INTO F

			//THRESHOLD AREG_vertical_edges TO BINARY RESULT
			scamp5_kernel_begin();
				sub(E,AREG_vertical_edges,F);
				where(E);
					//IN PES WHERE AREG_vertical_edges > edge_threshold, FLAG == 1
					MOV(DREG_vertical_edges,FLAG);
				all();
			scamp5_kernel_end();

			//THRESHOLD AREG_horizontal_edges TO BINARY RESULT
			scamp5_kernel_begin();
				sub(E,AREG_horizontal_edges,F);
				where(E);
					//IN PES WHERE AREG_horizontal_edges > edge_threshold, FLAG == 1
					MOV(DREG_horizontal_edges,FLAG);
				all();
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//COMBINE THRESHOLDED HORIZONTAL & VERTICAL EDGE RESULTS

			scamp5_kernel_begin();
				MOV(DREG_combined_edges,DREG_vertical_edges);//COPY VERTICAL EDGE RESULT
				OR(DREG_combined_edges,DREG_horizontal_edges);//OR WITH HORIZONTAL EDGE RESULT
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT RESULTS STORED IN VARIOUS REGISTERS

		   	output_timer.reset();

			if(!output_only_combined_edges)
			{
				output_4bit_image_via_DNEWS(AREG_image,display_00);
				output_4bit_image_via_DNEWS(AREG_vertical_edges,display_01);
				output_4bit_image_via_DNEWS(AREG_horizontal_edges,display_02);
				scamp5_output_image(DREG_vertical_edges,display_11);
				scamp5_output_image(DREG_horizontal_edges,display_12);
			}
			scamp5_output_image(DREG_combined_edges,display_10);

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

