#include <scamp5.hpp>
#include <vector>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch sum_timer;

const areg_t AREG_image = A;
const areg_t AREG_vertical_edges = B;
const areg_t AREG_horizontal_edges = C;

const dreg_t DREG_keyframe_edges = S0;
const dreg_t DREG_combined_edges = S1;
const dreg_t DREG_vertical_edges = S2;
const dreg_t DREG_horizontal_edges = S3;

void threshold_F_into_S6_for_frame_blending(areg_t reg,vs_handle display, int frame_blend_window = 16);

int main()
{
    vs_init();

	VS_GUI_DISPLAY_STYLE(style_plot,R"JSON(
			{
				"plot_palette": "plot_4",
				"plot_palette_groups": 4
			}
			)JSON");

    int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("Stabilized Image",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("Captured Image",0,display_size,display_size);

	int alignment_x = 0; int alignment_y = 0;
	int vel_x = 0; int vel_y = 0;

	int use_inertia = 1;
	vs_gui_add_switch("use_inertia",1,&use_inertia);

	bool set_anchor_image = false;
	auto gui_btn_set_anchor_image = vs_gui_add_button("set_anchor_image");
	vs_on_gui_update(gui_btn_set_anchor_image,[&](int32_t new_value)
	{
		set_anchor_image = true;
   });

    int edge_threshold = 12;
    vs_gui_add_slider("edge_threshold x",0,127,edge_threshold,&edge_threshold);


    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //CAPTURE FRAME

			scamp5_kernel_begin();
				//A = pixel data of latest frame, F = intermediate result
				get_image(A,F);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//COMPUTE HORIZONTAL &  VERTICAL EDGE IMAGES

			scamp5_kernel_begin();
				//COPY IMAGE TO F AND SHIFT ONE "PIXEL" RIGHT
				bus(NEWS,AREG_image);
				bus(F,XW);
				bus(NEWS,F);
				bus(F,XW);

				//SAVE THE ABSOLUTE DIFFERENCE BETWEEN THE SHIFTED IMAGE AND ORIGINAL AS THE VERTICAL EDGES RESULT
				sub(F,F,AREG_image);
				abs(AREG_vertical_edges,F);

				//COPY IMAGE TO F AND SHIFT ONE "PIXEL" UP
				bus(NEWS,AREG_image);
				bus(F,XS);
				bus(NEWS,F);
				bus(F,XS);

				//SAVE THE ABSOLUTE DIFFERENCE BETWEEN THE SHIFTED IMAGE AND ORIGINAL AS THE HORIZONTAL EDGES RESULT
				sub(F,F,AREG_image);
				abs(AREG_horizontal_edges,F);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//THRESHOLD AREG EDGE DATA TO BINARY

			scamp5_in(F,edge_threshold);//LOAD THRESHOLD INTO F

			scamp5_kernel_begin();
				//THRESHOLD AREG_vertical_edges TO BINARY RESULT
				sub(E,AREG_vertical_edges,F);
				where(E);
					//IN PES WHERE AREG_vertical_edges > edge_threshold, FLAG == 1
					MOV(DREG_vertical_edges,FLAG);
				all();

				//THRESHOLD AREG_horizontal_edges TO BINARY RESULT
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


			int position_threshold = 48;
			if(abs(alignment_x) > position_threshold || abs(alignment_y) > position_threshold)
			{
				set_anchor_image = true;
			}
			if(set_anchor_image)
			{
				set_anchor_image = false;
				alignment_x = 0;
				alignment_y = 0;
				vel_x = 0;
				vel_y = 0;

				scamp5_kernel_begin();
					MOV(DREG_keyframe_edges,DREG_combined_edges);
				scamp5_kernel_end();
			}
			scamp5_kernel_begin();
				REFRESH(DREG_keyframe_edges);
			scamp5_kernel_end();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //IMAGE ALIGNMENT
		//This code updates the "position" at which the captured image aligns with the stored key-frame

			if(use_inertia)
			{
				alignment_x += vel_x;
				alignment_y += vel_y;
			}

			//Copy the latest edge frame and shift it to the previous position determined to align with the edge key-frame
			scamp5_kernel_begin();
				MOV(S4,S1);
			scamp5_kernel_end();
			scamp5_shift(S4,alignment_x,alignment_y);

			//The  directions to shift and test for better alignment (i.e. noshift,left,right,down,up)
			int test_shift_directions[][5] =
			{
			    {0,0},
				{1,0},
				{-1, 0},
				{0,1},
				{0, -1},
			};

			//Array to store the global summations for each tested shift
			int direction_global_sums[5];

			//Test each shift
			for (int n = 0; n < 5; n++)
			{
				int x = test_shift_directions[n][0];
				int y = test_shift_directions[n][1];

				//Copy the shifted edge frame & apply the test shift to it
				scamp5_kernel_begin();
					MOV(S6,S4);
				scamp5_kernel_end();
				scamp5_shift(S6,x,y);

				scamp5_in(F,127);
				scamp5_kernel_begin();
					bus(D,F);//D = -127

					AND(S5,S6,S0);//S5 == 1 where the edges of S6 & DREG_keyframe_edges overlap
					WHERE(S5);
						mov(D,F);//D = 127 in PEs where the edges of
					ALL();
				scamp5_kernel_end();
				direction_global_sums[n] = scamp5_global_sum_64(D);
			}

			if(direction_global_sums[0] > direction_global_sums[1] &&
					direction_global_sums[0] > direction_global_sums[2] &&
					direction_global_sums[0] > direction_global_sums[3] &&
					direction_global_sums[0] > direction_global_sums[4])
			{

			}
			else
			{
				if(direction_global_sums[1] < direction_global_sums[2])
				{
					alignment_x--;
					vel_x--;
				}
				else
				{
					alignment_x++;
					vel_x++;
				}

				if(direction_global_sums[3] < direction_global_sums[4])
				{
					alignment_y--;
					vel_y--;
				}
				else
				{
					alignment_y++;
					vel_y++;
				}
			}

	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   //OUTPUT

			output_timer.reset();
			{
//				scamp5_output_image(S6,display_01);


				scamp5_kernel_begin();
					 MOV(S6,S1);
				scamp5_kernel_end();
				scamp5_shift(S6,alignment_x+1,alignment_y);
				scamp5_kernel_begin();
					 OR(S5,DREG_keyframe_edges,S6);
				scamp5_kernel_end();
				scamp5_output_image(S6,display_00);

				scamp5_output_image(S1,display_01);



//				scamp5_output_image(DREG_keyframe_edges,display_02);
			}
			int image_output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (image_output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}

