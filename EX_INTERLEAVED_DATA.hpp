#include <scamp5.hpp>
using namespace SCAMP5_PE;
#include "MISC/MISC_FUNCS.hpp"

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("Captured Image",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("Interlaced Image Data",0,display_size,display_size);
    vs_handle display_02 = vs_gui_add_display("Retrieved Image Data",0,display_size*2,display_size);

    vs_handle display_10 = vs_gui_add_display("Interlaced Data Bit 0",display_size,display_size+0,1);
    vs_handle display_11 = vs_gui_add_display("Interlaced Data Bit 1",display_size,display_size+1,1);
    vs_handle display_12 = vs_gui_add_display("Interlaced Data Bit 2",display_size,display_size+2,1);
    vs_handle display_13 = vs_gui_add_display("Interlaced Data Bit 3",display_size,display_size+3,1);

    int triggered_image_store = -1;

    vs_handle gui_btn_store_image_0 = vs_gui_add_button("store_image_0");
	vs_on_gui_update(gui_btn_store_image_0,[&](int32_t new_value)
	{
		triggered_image_store = 0;
    });

    vs_handle gui_btn_store_image_1 = vs_gui_add_button("store_image_1");
	vs_on_gui_update(gui_btn_store_image_1,[&](int32_t new_value)
	{
		triggered_image_store = 1;
    });

    vs_handle gui_btn_store_image_2 = vs_gui_add_button("store_image_2");
	vs_on_gui_update(gui_btn_store_image_2,[&](int32_t new_value)
	{
		triggered_image_store = 2;
    });

    vs_handle gui_btn_store_image_3 = vs_gui_add_button("store_image_3");
	vs_on_gui_update(gui_btn_store_image_3,[&](int32_t new_value)
	{
		triggered_image_store = 3;
    });


    int shown_image_index = 0;
    vs_gui_add_slider("shown_image_index: ",0,3,shown_image_index,&shown_image_index);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	scamp5_kernel_begin();
			get_image(A,F);
		scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //STORE 4BIT IMAGE USING INTERLACED STORAGE

			scamp5_in(F,64);
        	if(triggered_image_store != -1)
        	{
        		//Load into S6 the pattern of the subgrid containing the PEs into which to store image data
        		int col = triggered_image_store % 2 == 0 ? 0 : 1;
        		int row = triggered_image_store / 2 == 0 ? 0 : 1;
        		scamp5_load_pattern(S6, row, col, 254, 254);

        		scamp5_in(F,127);
				scamp5_kernel_begin();
					mov(D,A);

					where(D);// where D > 0

						//Update bit 3 (S0)
						SET(RF);
						MOV(RP,S0); //Copy S0 to RP in all PEs
						MOV(RF,S6);
						MOV(RP,FLAG);//Copy FLAG into RP in PEs of subgrid, i.e. PEs where S6 = 1
						MOV(S0,RP);

						NOT(RF,FLAG);
					WHERE(RF);
						add(D,D,F);//where D < 0, add 127 to it
					all();

					//////////////////////////////////////////////

					divq(E,F);
					mov(F,E);//F ~= 64
					sub(E,D,F);
					where(E);// where D > +64
						mov(D,E);

						//Update bit 2 (S1)
						SET(RF);
						MOV(RP,S1); //Copy S1 to RP in all PEs
						MOV(RF,S6);
						MOV(RP,FLAG);//Copy FLAG into RP in PEs of subgrid, i.e. PEs where S6 = 1
						MOV(S1,RP);
					all();

					//////////////////////////////////////////////

					divq(E,F);
					mov(F,E);//F ~= 32
					sub(E,D,F);
					where(E);// where D > +32
						mov(D,E);

						//Update bit 1 (S2)
						SET(RF);
						MOV(RP,S2); //Copy S2 to RP in all PEs
						MOV(RF,S6);
						MOV(RP,FLAG);//Copy FLAG into RP in PEs of subgrid, i.e. PEs where S6 = 1
						MOV(S2,RP);
					all();

					//////////////////////////////////////////////

					divq(E,F);
					mov(F,E);//F ~= 16
					sub(E,D,F);
					where(E);// where D > 16
						mov(D,E);

						//Update bit 0 (S3)
						SET(RF);
						MOV(RP,S3); //Copy S3 to RP in all PEs
						MOV(RF,S6);
						MOV(RP,FLAG);//Copy FLAG into RP in PEs of subgrid, i.e. PEs where S6 = 1
						MOV(S3,RP);
					all();
				scamp5_kernel_end();

        		triggered_image_store = -1;
        	}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//RETRIEVED 4BIT IMAGE DATA FROM INTERLACED DREG STORAGE

			//Create interleaved analogue image (in C) from the interleaved image data stored in S0,S1,S2,S3
			{
				scamp5_in(C,-127);
				scamp5_in(E,127);
				scamp5_kernel_begin();
					WHERE(S0);//Retrieve Bit 3
						add(C,C,E);
					ALL();

					divq(D,E);//D = 64
					WHERE(S1);//Retrieve Bit 2
						add(C,C,D);
					ALL();

					divq(E,D);//E = 32
					WHERE(S2);//Retrieve Bit 1
						add(C,C,E);
					ALL();

					divq(D,E);//D = 16
					WHERE(S3);//Retrieve Bit 0
						add(C,C,D);
					ALL();
				scamp5_kernel_end();
			}

			//Extract a specific image from the interleaved image data into F
			{
				int col = shown_image_index % 2 == 0 ? 0 : 1;
				int row = shown_image_index / 2 == 0 ? 0 : 1;
				scamp5_load_pattern(S6, row, col, 254, 254);
				scamp5_in(D,0);
				scamp5_kernel_begin();
					mov(F,D);
					//Copy image data from selected subgrid
					WHERE(S6);
						mov(D,C);
						mov(F,C);
					ALL();

					//spread image data copied from subgrid to fill all PEs
					bus(NEWS,F);
					bus(F,XE);
					add(D,D,F);//Spread to fill horizontally

					mov(F,D);
					bus(NEWS,F);
					bus(F,XS);
					add(D,D,F);//Spread to fill vertically
				scamp5_kernel_end();
			}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			output_4bit_image_via_DNEWS(A,display_00);
			output_4bit_image_via_DNEWS(C,display_01);
			output_4bit_image_via_DNEWS(D,display_02);


			scamp5_output_image(S3,display_10);
			scamp5_output_image(S2,display_11);
			scamp5_output_image(S1,display_12);
			scamp5_output_image(S0,display_13);
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


