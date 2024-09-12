#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_shift_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("DREG content",0,0,disp_size);
		auto display_01 = vs_gui_add_display("Content after expansion & erosion",0,disp_size,disp_size);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    int expansion_steps = 20;
	    vs_gui_add_slider("expansion_steps",0,50,expansion_steps,&expansion_steps);

	    int erosion_steps = 0;
	    vs_gui_add_slider("erosion_steps",0,50,erosion_steps,&erosion_steps);

	    int box1_x, box1_y, box1_width, box1_height;
	    vs_gui_add_slider("box1 x: ", 0, 255, 82, &box1_x);
	    vs_gui_add_slider("box1 y: ", 0, 255, 96, &box1_y);
	    vs_gui_add_slider("box1 width: ", 0, 128, 37, &box1_width);
	    vs_gui_add_slider("box1 height: ", 0, 128, 37, &box1_height);

		int use_stepwise = 1;
		vs_gui_add_switch("1 dir per step",use_stepwise == 1,&use_stepwise);


    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATE SOME CONTENT IN DREG S0 TO DEMONSTRATE EROSION / EXPANSION UPON

			DREG_load_centered_rect(S6,box1_x,box1_y,box1_width,box1_height);
			DREG_load_centered_rect(S5,196,146,13,129);
			scamp5_load_point(S4,200,60);
  			scamp5_kernel_begin();
  				MOV(S0,S6);
  				OR(S0,S5);
  				OR(S0,S4);
  				MOV(S2,S0);//make a copy of S0
  			scamp5_kernel_end();


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//PERFORM DREG EXPANSION AND EROSION

  			areg_shift_timer.reset();



  			//Perform expansion steps
  			{
//  				if(expansion_steps > 0)
//  				{
//  					scamp5_kernel_begin();
//						DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
//						OR(S0,S6);//Combine expanded locations with S0 itself
//					scamp5_kernel_end();
//  				}

  				if(use_stepwise)
  				{
  					scamp5_kernel_begin();
						SET(RN,RS,RE,RW);
					scamp5_kernel_end();
					for(int n = 0 ; n < expansion_steps/4; n++)
					{
						scamp5_kernel_begin();
							DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
							OR(S0,S6);//Combine expanded locations with S0 itself
						scamp5_kernel_end();
					}

					int substeps = expansion_steps%4;
					if(substeps > 0)
					{
						scamp5_kernel_begin();
							CLR(RN,RS,RE,RW);
							SET(RE);
							DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
							OR(S0,S6);//Combine expanded locations with S0 itself
						scamp5_kernel_end();
						substeps--;
					}
					if(substeps > 0)
					{
						scamp5_kernel_begin();
							CLR(RE);
							SET(RN);
							DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
							OR(S0,S6);//Combine expanded locations with S0 itself
						scamp5_kernel_end();
						substeps--;
					}
					if(substeps > 0)
					{
						scamp5_kernel_begin();
							CLR(RN);
							SET(RW);
							DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
							OR(S0,S6);//Combine expanded locations with S0 itself
						scamp5_kernel_end();
						substeps--;
					}
  				}
  				else
  				{
  					scamp5_kernel_begin();
						SET(RN,RS,RE,RW);
					scamp5_kernel_end();
					for(int n = 0 ; n < expansion_steps; n++)
					{
						scamp5_kernel_begin();
							DNEWS0(S6,S0);//S6 will contain 1s at locations that S0 will expand into this step
							OR(S0,S6);//Combine expanded locations with S0 itself
						scamp5_kernel_end();
					}
  				}
  			}

  			//Perform erosion steps
  			{
				scamp5_kernel_begin();
					NOT(S1,S0);//invert DREG content
				scamp5_kernel_end();

				//expand inverted content
				for(int n = 0 ; n < erosion_steps ; n++)
				{
					scamp5_kernel_begin();
						DNEWS0(S6,S1);
						OR(S1,S6);
					scamp5_kernel_end();
				}

				scamp5_kernel_begin();
					NOT(S0,S1);//invert back again
				scamp5_kernel_end();
  			}


  			int time_spent_on_errode_expand = areg_shift_timer.get_usec();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			scamp5_output_image(S2,display_00);
			scamp5_output_image(S0,display_01);
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("time spent on erode//exapand %d \n",time_spent_on_errode_expand);
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }
    return 0;
}

