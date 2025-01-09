#include <scamp5.hpp>
#include <random>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("Image",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("Thresholded Image",0,display_size,display_size);
    vs_handle display_02 = vs_gui_add_display("Extracted Shapes",0,display_size*2,display_size);

	int threshold_value = 0;
	vs_gui_add_slider("threshold value",-127,127,threshold_value,&threshold_value);

	int shapes_to_extract = 16;
	vs_gui_add_slider("shapes to extract",0,64,shapes_to_extract,&shapes_to_extract);

	int erosion_steps = 1;
	vs_gui_add_slider("minimum shape size",0,5,erosion_steps,&erosion_steps);

	int invert_input = 1;
	vs_gui_add_switch("extract dark shapes",invert_input == 1,&invert_input);

	int flood_edges = 1;
	vs_gui_add_switch("filter image edges",flood_edges == 1,&flood_edges);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   //CAPTURE FRAME AND PERFORM THRESHOLDING

           	//load threshold value into C across all PEs
   			scamp5_in(C,threshold_value);

   			scamp5_kernel_begin();
   				//A = pixel data of latest frame, F = intermediate result
   				get_image(A,F);

   				//C = (A - C) == (latest frame pixel - threshold)
   				sub(F,A,C);

   				//sets FLAG = 1 in PEs where F > 0 (i.e. where A > C), else FLAG = 0
   				where(F);
   					//copy FLAG into S0, creating a thresholded image
   					MOV(S0,FLAG);
   					MOV(S1,FLAG);
   				all();
   			scamp5_kernel_end();

   			if(invert_input)
   			{
   				//Invert thresholded image
   				scamp5_kernel_begin();
					NOT(S0,S1);
					MOV(S1,S0);
				scamp5_kernel_end();
   			}



		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//FLOOD THRESHOLDED IMAGE FROM EDGES TO ENSURE ONLY SHAPES ENTIRELY WITHIN THE FRAME ARE EXTRACTED

			if(flood_edges)
			{
				//Perform Flooding using native instructions
				{
					scamp5_kernel_begin();
						SET (RN,RS,RE,RW);//Set all DNEWS register so flooding is performed in all directions across whole processor array

						SET(RF);//Reset digital Flag = 1 across whole processor array, as operations targeting RZ are Flagged by RF
						CLR(RP);//Copy loaded point into RZ
						MOV(RF,S0);//Copy the content of S0 into RF to act as the Mask during flooding
					scamp5_kernel_end();

					//Perform flooding iterations
					int flood_iterations = 10;
					for(int n = 0 ; n < flood_iterations ; n++)
					{
						scamp5_kernel_begin();
							PROP1();
						scamp5_kernel_end();
					}

					scamp5_kernel_begin();
						NOT(S6,RP);
						AND(S0,S0,S6);
					scamp5_kernel_end();
				}
			}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//PERFORM ERROSION STEPS ON THRESHOLDED IMAGE

			if(erosion_steps > 0)
			{
				//invert DREG content
				scamp5_kernel_begin();
					NOT(S5,S0);
				scamp5_kernel_end();
				//expand inverted content
				for(int n = 0 ; n < erosion_steps ; n++)
				{
					scamp5_kernel_begin();
						DNEWS0(S6,S5);
						OR(S5,S6);
					scamp5_kernel_end();
				}
				//invert back again
				scamp5_kernel_begin();
					NOT(S0,S5);
				scamp5_kernel_end();
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//EXTRACT SHAPES ONE BY ONE
		//STEPs 1."scamp5_scan_events" 2."scamp5_load_point" 3.Flood from point 4.process flooded shape 5.repeat

			scamp5_kernel_begin();
				CLR(S2);//S2 Will be used to store all extracted shapes
			scamp5_kernel_end();
			int shapes_extracted = 0;
   			for(int s = 0 ; s < shapes_to_extract ; s++)
   			{
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//PERFORM FLOODING OF S0, ORGINATING FROM A SPECIFIC POINT

				uint8_t event_data_array [2];
				//Scan Events from S0 register plane
				//Examines the S0 register of each PE, returning the locations of PEs in which S0 == 1
				//The locations of these PEs will be written to the array "event_data_array" as (x,y) coordinate pairs
				//Once all events have been scanned will write "(0,0)" coordinates into the array
				scamp5_scan_events(S0,event_data_array,1);

				int event_xpos = event_data_array[0];
				int event_ypos = event_data_array[1];

				//Check that the coordinates returned are not (0,0)
				if(event_xpos != 0 || event_ypos != 0)
				{
					shapes_extracted++;
					scamp5_load_point(S6,event_ypos,event_xpos);

					//Perform Flooding using native instructions
					{
						scamp5_kernel_begin();
							SET (RN,RS,RE,RW);//Set all DNEWS register so flooding is performed in all directions across whole processor array

							SET(RF);//Reset digital Flag = 1 across whole processor array, as operations targeting RZ are Flagged by RF
							MOV(RP,S6);//Copy loaded point into RZ
							MOV(RF,S1);//Copy the content of S0 into RF to act as the Mask during flooding
						scamp5_kernel_end();

						//Perform flooding iterations
						int flood_iterations = 10;
						for(int n = 0 ; n < flood_iterations ; n++)
						{
							scamp5_kernel_begin();
								PROP0();
							scamp5_kernel_end();
						}

						scamp5_kernel_begin();
							MOV(S6,RP);//Copy result of flooding, ie the shape
							OR(S2,S6);//Add the extracted shape to S2
						scamp5_kernel_end();
					}


					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//REMOVE THE FLOODED SHAPE FROM THE DREG PLANE

						scamp5_kernel_begin();
							NOT(S5,S6);//Invert DREG plane holding extracted shape and store in S5
							AND(S0,S0,S5);//Eliminate extracted shape from S0 by performing AND operation with S5
						scamp5_kernel_end();

					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//OUTPUT THE BOUNDING BOX OF THE FLOODED SHAPE

						uint8_t bb_data [4];
						scamp5_scan_boundingbox(S6,bb_data);//Retrieve the bounding box of the shape in S6

						scamp5_display_boundingbox(display_00,bb_data,1);
						scamp5_display_boundingbox(display_01,bb_data,1);
						scamp5_display_boundingbox(display_02,bb_data,1);

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
			}


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //OUTPUT IMAGES

        	output_timer.reset();

			scamp5_output_image(A,display_00);
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

