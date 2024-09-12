#include <scamp5.hpp>
#include <random>
#include "MISC/MISC_FUNCS.hpp"
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
	    auto display_01 = vs_gui_add_display("Mask",0,disp_size,disp_size);
	    auto display_02 = vs_gui_add_display("Source After Flooding",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

		bool generate_boxes = true;
		vs_handle gui_button_generate_boxes = vs_gui_add_button("generate boxes");
		vs_on_gui_update(gui_button_generate_boxes,[&](int32_t new_value)//function that will be called whenever button is pressed
		{
			generate_boxes = true;//trigger generation of boxes
	    });

	    //control the box drawn into DREG & used as the flooding Source
	    int flood_source_box_x, flood_source_box_y,flood_source_box_width,flood_source_box_height;
		vs_gui_add_slider("flood_source_box_x: ",1,255,64,&flood_source_box_x);
		vs_gui_add_slider("flood_source_box_y: ",1,255,125,&flood_source_box_y);
		vs_gui_add_slider("flood_source_box_width: ",1,100,20,&flood_source_box_width);
		vs_gui_add_slider("flood_source_box_height: ",1,100,20,&flood_source_box_height);

		//The number of iterations performed, determines the extent/distance flooding can reach
	    int flood_iterations = 10;
	    vs_gui_add_slider("flood_iterations", 0,20,flood_iterations,&flood_iterations);

	    //Select if the borders of the PE array/image act as Sources of 1s during flooding
		int flood_from_borders = 0;
		vs_gui_add_switch("flood_from_borders",false,&flood_from_borders);

		//Add switches for selecting the content of the RN,RS,RE,RW DREG
		//These registers control the directions from which each PE can be flooded with 1s from
		int set_RN = 1;
		vs_gui_add_switch("set_RN",set_RN == 1,&set_RN);
		int set_RS = 1;
		vs_gui_add_switch("set_RS",set_RS == 1,&set_RS);
		int set_RE = 1;
		vs_gui_add_switch("set_RE",set_RE == 1,&set_RE);
		int set_RW = 1;
		vs_gui_add_switch("set_RW",set_RW == 1,&set_RW);

		//Switch between performing flooding using native commands or with the library Macro
		int use_api = 0;
		vs_gui_add_switch("use_api",use_api == 1,&use_api);

		//Use a fixed single kernel flooding routine to demonstrate maximum speed
		int single_kernel_flood_example = 0;
		vs_gui_add_switch("use_single_kernel",single_kernel_flood_example == 1,&single_kernel_flood_example);

		int negate_mask = 0;
		vs_gui_add_switch("negate_mask",negate_mask == 1,&negate_mask);

		//Setup objects for random number generation
		//This code looks like nonsense as the std classes here overload the function call operator...
		std::random_device rd;//A true random number
		std::mt19937 gen(rd()); // Mersenne Twister pseudo random number generator, seeded with a true random number
		int random_min_value = 0;
		int random_max_value = 255;
		//Create distribution object that can be used to map a given random value to a value of the distribution
		std::uniform_int_distribution<> distr(random_min_value, random_max_value);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
              //GENERATES RANDOM BOXES IN DREG S0 IF BUTTON WAS PRESSED (OR PROGRAM JUST STARTED)

      			if(generate_boxes)
      			{
      				const int boxes_to_add = 50;
      				const int boxes_to_subtract = 25;

      				//Generate a set of random rectangles across DREG plane S0
      				{
      					scamp5_kernel_begin();
      						CLR(S0); //Clear content of S0
      					scamp5_kernel_end();
      					for(int n = 0 ; n < boxes_to_add ; n++)
      					{
      						//Load box of random location and dimensions into S5
      						int pos_x = distr(gen);
      						int pos_y = distr(gen);
      						int width = 1+distr(gen)/6;
      						int height = 1+distr(gen)/6;
      						DREG_load_centered_rect(S5,pos_x,pos_y,width,height);
      						scamp5_kernel_begin();
      							OR(S0,S5);//Add box in S5 to content of S0
      						scamp5_kernel_end();
      					}
      				}
//
//      				//Generate another set of random rectangles across DREG plane S1
//      				scamp5_kernel_begin();
//      					CLR(S6); //Clear content of S1
//      				scamp5_kernel_end();
//      				for(int n = 0 ; n < boxes_to_subtract ; n++)
//      				{
//      					//Load box of random location and dimensions into S5
//      					int pos_x = distr(gen);
//      					int pos_y = distr(gen);
//      					int width = 1+distr(gen)/5;
//      					int height = 1+distr(gen)/5;
//      					DREG_load_centered_rect(S5,pos_x,pos_y,width,height);
//      					scamp5_kernel_begin();
//      						OR(S6,S5);//Add box in S5 to content of S0
//      					scamp5_kernel_end();
//      				}
//
//      				//Subtract the rectangles in S1 from those of S2
//      				scamp5_kernel_begin();
//      					NOT(S5,S6);//S5 = Inverted content of S1
//      					AND(S0,S5,S0);//perform AND to "Subtract" S1 rectangles from S0
//      				scamp5_kernel_end();

      				generate_boxes = false;
      			}

      			//Create a copy or negated copy of these rectangles to use as the flooding mask
      			if(negate_mask)
      			{
      				scamp5_kernel_begin();
      					NOT(S4,S0);
      				scamp5_kernel_end();
      			}
      			else
      			{
      				scamp5_kernel_begin();
						MOV(S4,S0);
					scamp5_kernel_end();
      			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //DRAW CONTENT INTO DIGITAL REGISTERS FOR FLOODING EXAMPLE

        	drawing_timer.reset();

        	//Draw content to use as the Flooding Source, Flooding orginates from PEs in which Source = 1
			DREG_load_centered_rect(S1,flood_source_box_x,flood_source_box_y,flood_source_box_width,flood_source_box_height);
			scamp5_kernel_begin();
				MOV(S3,S1);
			scamp5_kernel_end();

			int time_spent_drawing = drawing_timer.get_usec();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SET DNEWS DREG WHICH CONTROL THE DIRECTIONS IN WHICH FLOODING MAY OCCUR WITHIN EACH PE

			scamp5_kernel_begin();
				CLR(RN,RS,RE,RW);//Set all DNEWS DREG = 0 in all PEs
			scamp5_kernel_end();

			if(set_RN)
			{
				scamp5_kernel_begin();
					SET(RN);//Set all RN = 1 in all PEs
				scamp5_kernel_end();
			}
			if(set_RS)
			{
				scamp5_kernel_begin();
					SET(RS);//Set all RS = 1 in all PEs
				scamp5_kernel_end();
			}
			if(set_RE)
			{
				scamp5_kernel_begin();
					SET(RE);//Set all RE = 1 in all PEs
				scamp5_kernel_end();
			}
			if(set_RW)
			{
				scamp5_kernel_begin();
					SET(RW);//Set all RW = 1 in all PEs
				scamp5_kernel_end();
			}




		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//DEMONSTRATE FLOODING

			flooding_timer.reset();

			//FLOODING USING SCAMP LIBRARY FUNCTION
			if(!use_api && !single_kernel_flood_example)
			{
				//Perform flooding using native instructions
				//RF acts as the Source register, PEs where RF = 1 will spread 1s into RF registers of neighbouring PEs during flooding
				//RP acts as the Mask register, which restricts flooding to only those PEs where RP = 1
				scamp5_kernel_begin();
					MOV(RP,S1);//Copy the content of S1 into the Flooding Source RP
					MOV(RF,S4);//Copy the content of S2 into the Flooding Mask RF
				scamp5_kernel_end();

				if(flood_from_borders)
				{
					for(int n = 0 ; n < flood_iterations ; n++)
					{
						scamp5_kernel_begin();
							PROP1();//Propagate 1s from both Flooding Source and boundaries of the Array
						scamp5_kernel_end();
					}
				}
				else
				{
					for(int n = 0 ; n < flood_iterations ; n++)
					{
						scamp5_kernel_begin();
							PROP0();//Propagate 1s from both Flooding Source
						scamp5_kernel_end();
					}
				}

				scamp5_kernel_begin();
					MOV(S1,RP);//Copy the result of Flooding into S1
					SET(RF);//Reset RF = 1 in all PEs
				scamp5_kernel_end();
			}




			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




			//FLOODING USING A SINGLE SCAMP KERNEL
			if(!use_api && single_kernel_flood_example)
			{
				//Example of performing flooding using native instructions
				//By only using a single Kernel this code is more efficient however the number of flooding iterations is fixed
				scamp5_kernel_begin();
					MOV(RP,S1);//Copy the content of S1 into the Flooding Source RP
					MOV(RF,S4);//Copy the content of S2 into the Flooding Mask RF
					for(int n = 0 ; n < 12 ; n++)
					{
						PROP0();//Propagate 1s from both Flooding Source
					}
					MOV(S1,RP);//Copy the result of Flooding into S1
					SET(RF);//Reset RF = 1 in all PEs
				scamp5_kernel_end();
			}



			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




			//FLOODING USING SCAMP LIBRARY FUNCTION
			if(use_api)
			{
				//Perform flooding using provided scamp library function
				//Floods the Source DREG (S1), restricted by the Mask DREG, for a given number of steps/iterations
				scamp5_flood(S1,S4,flood_from_borders,flood_iterations);
			}


			int time_spent_flooding = flooding_timer.get_usec();


			scamp5_kernel_begin();
				REFRESH(S0);
				REFRESH(S1);
				REFRESH(S2);
			scamp5_kernel_end();


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//OUTPUT THE BOUNDING BOX OF THE FLOODED SHAPE

				uint8_t bb_data [4];
				scamp5_output_boundingbox(S1,display_00,bb_data);
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

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();

			scamp5_output_image(S3,display_00);
			scamp5_output_image(S0,display_01);
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
