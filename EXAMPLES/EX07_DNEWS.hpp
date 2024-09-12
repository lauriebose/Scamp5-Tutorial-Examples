#include <scamp5.hpp>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP IMAGE DISPLAYS

		int disp_size = 2;
		auto display_00 = vs_gui_add_display("S0",0,0,disp_size);
		auto display_01 = vs_gui_add_display("S0 after DNEWS operations",0,disp_size,disp_size);
		auto display_10 = vs_gui_add_display("RN - Take From North Neighbour",0,disp_size*2,1);
		auto display_11 = vs_gui_add_display("RS - Take From South Neighbour",0,disp_size*2+1,1);
		auto display_12 = vs_gui_add_display("RE - Take From East Neighbour",1,disp_size*2,1);
		auto display_13 = vs_gui_add_display("RW - Take From West Neighbour",1,disp_size*2+1,1);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //SETUP GUI ELEMENTS & CONTROLLABLE VARIABLES

	    int DNEWS_iterations = 60;
	    vs_gui_add_slider("DNEWS_iterations",0,128,DNEWS_iterations,&DNEWS_iterations);

	    //Setup switches for setting/clearing the content of the RN,RS,RE,RW DREG, which control the behaviour of DNEWS
		int set_RN = 0;
		vs_gui_add_switch("set_RN",set_RN == 1,&set_RN);
		int set_RS = 1;
		vs_gui_add_switch("set_RS",set_RS == 1,&set_RS);
		int set_RE = 0;
		vs_gui_add_switch("set_RE",set_RE == 1,&set_RE);
		int set_RW= 0;
		vs_gui_add_switch("set_RW",set_RW == 1,&set_RW);

		//for toggling between using "DNEWS0" or "DNEWS1"
		int use_DNEWS1 = 0;
		vs_gui_add_switch("use_DNEWS1",use_DNEWS1 == 1,&use_DNEWS1);


    //CONTINOUS FRAME LOOP
    while(true)
    {
        frame_timer.reset();//reset frame_timer

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //GENERATE DATA IN DREG S0 TO DEMONSTRATE DNEWS OPERATION UPON

			//generate 4 boxes and 1 point in various DREG
			scamp5_load_point(S6,96,96);
			DREG_load_centered_rect(S5,188,118,32,32);
			DREG_load_centered_rect(S4,48,128,32,32);
			DREG_load_centered_rect(S3,10,125,10,200);
			DREG_load_centered_rect(S2,125,180,180,20);

			//combine S6,S5,S4 contents together in S0
			scamp5_kernel_begin();
				MOV(S0,S6);
				OR(S0,S5);
				OR(S0,S4);
				OR(S0,S3);
				OR(S0,S2);
			scamp5_kernel_end();

        	//make a copy of S0 in S1 to display for comparison display later
        	scamp5_kernel_begin();
				MOV(S1,S0);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//PERFORM DNEWS OPERATIONS ON CONTENT OF S0

			//Clear all DREG controlling DNEWS behaviour
  			scamp5_kernel_begin();
				CLR(RN,RS,RE,RW);//sets each DREG to 0 in all PEs
			scamp5_kernel_end();

			//Separately set each the DREG controlling DNEWS behaviour if toggled in the gui
			if(set_RN)
			{
				scamp5_kernel_begin();
					SET(RN);//sets DREG RN to 1 in all PES
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


			//Repeatedly perform DNEWS operations on S0 for the selected number of iterations
			for(int n = 0 ; n < DNEWS_iterations ; n++)
			{
				scamp5_kernel_begin();
					DNEWS0(S6,S0);//S6 = DNEWS(S0)
					MOV(S0,S6);//Copy the results in S6 back into S0
				scamp5_kernel_end();
			}


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();

			scamp5_output_image(S1,display_00);//display original unaltered DREG content
			scamp5_output_image(S0,display_01);//display content after applying DNEWS

			//display the content of registers controlling DNEWS behavior
			scamp5_output_image(RN,display_10);
			scamp5_output_image(RS,display_11);
			scamp5_output_image(RE,display_12);
			scamp5_output_image(RW,display_13);

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

