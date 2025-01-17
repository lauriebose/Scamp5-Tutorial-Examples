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

const dreg_t DREG_keyframe_edge_image = S0;
const dreg_t DREG_current_edge_image = S1;
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

    //The variables used to store how the current image should be shifted to align it with the stored key-frame
    //Note this is a very simple method in that it only aligns via translation, and does not account for rotation & scaling
	int alignment_x = 0; int alignment_y = 0;
	
	//Varibles used to keep track of alignment "velocity", allows more robust alignment under motion by estimating the new alignment each frame using this "velocity"
	int alignment_vel_x = 0; int alignment_vel_y = 0;

	int use_velocity = 1;
	vs_gui_add_switch("use_velocity",1,&use_velocity);

	bool set_anchor_image = false;
	auto gui_btn_set_anchor_image = vs_gui_add_button("set_anchor_image");
	vs_on_gui_update(gui_btn_set_anchor_image,[&](int32_t new_value)
	{
		set_anchor_image = true;
   });

    int edge_threshold = 12;
    vs_gui_add_slider("edge_threshold x",0,127,edge_threshold,&edge_threshold);

    int alignment_strength = 0;
    int minimum_alignment_strength = 2000;
    vs_gui_add_slider("minimum_alignment_strength",0,4000,minimum_alignment_strength,&minimum_alignment_strength);

	int show_current_edge_image = 1;
	vs_gui_add_switch("show_captured_image",1,&show_current_edge_image);

    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        if(alignment_strength < minimum_alignment_strength)
        {
        	set_anchor_image = true;
        	vs_post_text("insufficient edges");
        }
        alignment_strength = 0;

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
				MOV(DREG_current_edge_image,DREG_vertical_edges);//COPY VERTICAL EDGE RESULT
				OR(DREG_current_edge_image,DREG_horizontal_edges);//OR WITH HORIZONTAL EDGE RESULT
			scamp5_kernel_end();

			if(set_anchor_image)
			{
				set_anchor_image = false;
				alignment_x = 0;
				alignment_y = 0;
				alignment_vel_x = 0;
				alignment_vel_y = 0;

				scamp5_kernel_begin();
					MOV(DREG_keyframe_edge_image,DREG_current_edge_image);
				scamp5_kernel_end();
			}
			scamp5_kernel_begin();
				REFRESH(DREG_keyframe_edge_image);
			scamp5_kernel_end();

	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//ESTIMATE THE NEW ALIGNMENT USING "VELOCITY" INFOMATION

			if(use_velocity)
			{
				alignment_x += alignment_vel_x;
				alignment_y += alignment_vel_y;
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //TEST IMAGE ALIGNMENT USING VARIOUS SHIFTS
		//This code applies various shifts to the captured edge image
	    //It uses global summation to test the alignment strength between the edge image and key-frame resulting from each shift

			//Copy the latest edge frame and shift it according to the current alignment
			//This should roughly align it to the edge key-frame, allowing us to search for a better alignment by only shifting up/down/left/right 1 pixel
			scamp5_kernel_begin();
				MOV(S4,DREG_current_edge_image);
			scamp5_kernel_end();
			scamp5_shift(S4,alignment_x,alignment_y);

			//The shifts to test and see if they improve image alignment with the keyframe
			int test_shift_directions[][5] =
			{
			    {0,0},//no shift
				{1,0},//left
				{-1, 0},//right
				{0,1},//down
				{0, -1},//up
			};

			//Array to store the global summations for each tested shift
			int direction_global_sums[5];

			//Test each shift
			for (int n = 0; n < 5; n++)
			{
				int shift_x = test_shift_directions[n][0];
				int shift_y = test_shift_directions[n][1];

				//Copy the roughly aligned edge image, and then apply the test shift
				scamp5_kernel_begin();
					MOV(S6,S4);
				scamp5_kernel_end();
				scamp5_shift(S6,shift_x,shift_y);


				scamp5_in(F,127);
				scamp5_kernel_begin();
					//Create a binary image with 1s where the edge of the key-frame and shifted edge image overlap
					AND(S5,S6,DREG_keyframe_edge_image);

					//Convert this binary image to an analog image to feed into global summation
					bus(C,F);//== -127
					WHERE(S5);
						mov(C,F);//== 127 where edges of key-frame and shifted edge image overlap
					ALL();
				scamp5_kernel_end();

				//The result of this sum will be proportional to overlap of edges between the key-frame & shifted edge images
				//ie proportional to number of 1s in AND(S5,S6,DREG_keyframe_edge_image)
				//Higher summation indicates stronger alignment between key-frame & shifted edge image
				direction_global_sums[n] = scamp5_global_sum_64(C);

				//The highest value sum will have the best alignment, ie. the most overlap between the edge from the current frame and the key-frame
				if(alignment_strength < direction_global_sums[n])
				{
					alignment_strength = direction_global_sums[n];
				}
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//UPDATE alignment
		//Examine the global summation for each tested shift
		//Update alignment and velocity from those shifts which improved alignment with the key-frame

			if(direction_global_sums[0] > direction_global_sums[1] &&
				direction_global_sums[0] > direction_global_sums[2] &&
				direction_global_sums[0] > direction_global_sums[3] &&
				direction_global_sums[0] > direction_global_sums[4])
			{
				//The best alignment was when no shift was applied at all so nothing need be updated
			}
			else
			{
				//Determine which horizontal and vertical shift directions that improved alignment, and update the alignment & "velocity"

				//Did either shifting left or right result in a better alignment than not shifting at all?
				if(direction_global_sums[0] < direction_global_sums[1] || direction_global_sums[0] < direction_global_sums[2])
				{
					//Compare the summation from shifting left and right
					if(direction_global_sums[1] < direction_global_sums[2])
					{
						alignment_x--;
						alignment_vel_x--;
					}
					else
					{
						alignment_x++;
						alignment_vel_x++;
					}
				}

				//Did either shifting left or right result in a better alignment than not shifting at all?
				if(direction_global_sums[0] < direction_global_sums[3] || direction_global_sums[0] < direction_global_sums[4])
				{
					//Compare the summation from shifting up and down
					if(direction_global_sums[3] < direction_global_sums[4])
					{
						alignment_y--;
						alignment_vel_y--;
					}
					else
					{
						alignment_y++;
						alignment_vel_y++;
					}
				}
			}

	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   //OUTPUT

			output_timer.reset();
			{
//				scamp5_output_image(S6,display_01);


				scamp5_kernel_begin();
					 MOV(S6,DREG_current_edge_image);
				scamp5_kernel_end();
				scamp5_shift(S6,alignment_x+1,alignment_y);
				scamp5_kernel_begin();
					 OR(S5,DREG_keyframe_edge_image,S6);
				scamp5_kernel_end();
				scamp5_output_image(S6,display_00);

				if(show_current_edge_image)
				{
					scamp5_output_image(DREG_current_edge_image,display_01);
				}
				vs_post_text("alignment (%d,%d), velocity (%d,%d), strength %d \n",alignment_x,alignment_y,alignment_vel_x,alignment_vel_y,alignment_strength);
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

