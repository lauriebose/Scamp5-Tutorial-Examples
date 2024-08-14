#include <scamp5.hpp>
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
	    auto display_02 = vs_gui_add_display("Source",0,disp_size*2,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    //control the box drawn into DREG & used as the flooding Source
	    int box_posx, box_posy,box_width,box_height;
		vs_gui_add_slider("box_posx: ",1,255,64,&box_posx);
		vs_gui_add_slider("box_posy: ",1,255,125,&box_posy);
		vs_gui_add_slider("box_width: ",1,100,20,&box_width);
		vs_gui_add_slider("box_height: ",1,100,20,&box_height);

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

		int negate_mask = 1;
		vs_gui_add_switch("negate_mask",negate_mask == 1,&negate_mask);

    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //DRAW CONTENT INTO DIGITAL REGISTERS FOR FLOODING EXAMPLE

        	drawing_timer.reset();

        	//Draw content to use as the Flooding Source, Flooding orginates from PEs in which Source = 1
			DREG_load_centered_rect(S1,box_posx,box_posy,box_width,box_height);
			scamp5_kernel_begin();
				MOV(S3,S1);
			scamp5_kernel_end();

			//Draw content to use as the Flooding Mask, Flooding is restricted to PEs in which Mask = 1
			{
				scamp5_kernel_begin();
					CLR(S2);
				scamp5_kernel_end();

				scamp5_draw_begin(S2);
					scamp5_draw_circle(127,127,100);
					scamp5_draw_circle(127,127,50);
					scamp5_draw_line(10,0,10,150);
					scamp5_draw_line(10,150,100,150);
					if(negate_mask)
					{
						scamp5_draw_negate();
					}
				scamp5_draw_end();
			}

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
					SET(RF);//Required as instructions targeting RP are masked by RF
					MOV(RP,S1);//Copy the content of S1 into the Flooding Source S1
					MOV(RF,S2);//Copy the content of S2 into the Flooding Mask S2
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
					SET(RF);//Required as instructions targeting RP are masked by RF
					MOV(RP,S1);//Copy the content of S1 into the Flooding Source S1
					MOV(RF,S2);//Copy the content of S2 into the Flooding Mask S2
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
				scamp5_flood(S1,S2,flood_from_borders,flood_iterations);
			}


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
