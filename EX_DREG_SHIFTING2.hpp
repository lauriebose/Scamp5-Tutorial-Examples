#include <scamp5.hpp>
using namespace SCAMP5_PE;
#include "MISC/MISC_FUNCS.hpp"

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch dreg_shift_timer;
vs_stopwatch areg_shift_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("Shifted Image (NEWS)",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("Shifted ADC Image (DNEWS)",0,display_size,display_size);

    vs_handle display_bit0 = vs_gui_add_display("DREG ADC Bit 0",0,display_size*2,1);
    vs_handle display_bit1 = vs_gui_add_display("DREG ADC Bit 1",0,display_size*2+1,1);
    vs_handle display_bit2 = vs_gui_add_display("DREG ADC Bit 2",1,display_size*2,1);
    vs_handle display_bit3 = vs_gui_add_display("DREG ADC Bit 3",1,display_size*2+1,1);


    int shift_x = 0;
    vs_gui_add_slider("shift_x",-127,127,0,&shift_x);

    int shift_y = 0;
    vs_gui_add_slider("shift_y",-127,127,0,&shift_y);


    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	scamp5_kernel_begin();
			get_image(A,F);
			mov(B,A);
		scamp5_kernel_end();


        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM ADC (ANALOGUE TO DIGITAL CONVERSION), CONVERT CAPTURED IMAGE FROM AREG, TO 4BIT IMAGE IN 4 DREG
		//S0 = Bit 3 (Represents value ~128)
		//S1 = Bit 2 (Represents value ~64)
		//S2 = Bit 1 (Represents value ~32)
		//S3 = Bit 0 (Represents value ~16)
		//Take AREG values to range from -127 to 127

			scamp5_in(F,127);
			scamp5_kernel_begin();
				mov(D,A);//Create working copy of captured image

				//Compute Bit 3
				where(D);
					//PEs whose working image pixel is > 0
					//These PEs must have the 128 (Bit 3) DREG set
					MOV(S0,FLAG);//Update Bit 3

					NOT(RF,FLAG);
				WHERE(RF);
					//In PEs whose working image pixel is < 0, add 127
					//Can then use where() in later steps to to get the remaining bits for these PEs
					add(D,D,F);
				all();

				//Compute Bit 2
				divq(E,F);
				mov(F,E);//F ~= 64
				sub(E,D,F);
				where(E);//Where working image > 64
					mov(D,E);//Subtract 64
					MOV(S1,FLAG);//Update Bit 2
				all();

				//Compute Bit 1
				divq(E,F);
				mov(F,E);//F ~= 32
				sub(E,D,F);
				where(E);//Where working image  > 32
					mov(D,E);//Subtract 32
					MOV(S2,FLAG);//Update Bit 1
				all();

				//Compute Bit 0
				divq(E,F);
				mov(F,E);//F ~= 16
				sub(E,D,F);
				where(E);//Where working image > 16
					mov(D,E);//Subtract 16
					MOV(S3,FLAG);//Update Bit 0
				all();
			scamp5_kernel_end();




		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SHIFT STORED DIGITAL IMAGE DATA BY SHIFTING THE DREGS FOR EACH OF THE 4BITS

        	dreg_shift_timer.reset();

			//SHIFT DREGS HORIZONTALLY
			{
				scamp5_kernel_begin();
					CLR(RN,RS,RE,RW);
				scamp5_kernel_end();

				//Setup DNEWS control registers to shift in the correct direction
				if(shift_x > 0)
				{
					scamp5_kernel_begin();
						SET(RW);
					scamp5_kernel_end();
				}
				else
				{
					scamp5_kernel_begin();
						SET(RE);
					scamp5_kernel_end();
				}

				//Shift the DREGs for each of the 4 bits using DNEWS
				int shift_x_magnitude = abs(shift_x);
				for(int x = 0 ; x < shift_x_magnitude ; x++)
				{
					scamp5_kernel_begin();
						DNEWS0(S6,S3);
						MOV(S3,S6);//Shift Bit 0

						DNEWS0(S6,S2);
						MOV(S2,S6);//Shift Bit 1

						DNEWS0(S6,S1);
						MOV(S1,S6);//Shift Bit 2

						DNEWS0(S6,S0);
						MOV(S0,S6);//Shift Bit 3
					scamp5_kernel_end();
				}
			}

			//SHIFT DREGS VERTICALLY
			{
				scamp5_kernel_begin();
					CLR(RN,RS,RE,RW);
				scamp5_kernel_end();

				//Setup DNEWS control registers to shift in the correct direction
				if(shift_y > 0)
				{
					scamp5_kernel_begin();
						SET(RS);
					scamp5_kernel_end();
				}
				else
				{
					scamp5_kernel_begin();
						SET(RN);
					scamp5_kernel_end();
				}

				//Shift the DREGs for each of the 4 bits using DNEWS
				int shift_y_magnitude = abs(shift_y);
				for(int y = 0 ; y < shift_y_magnitude ; y++)
				{
					scamp5_kernel_begin();
						DNEWS0(S6,S3);
						MOV(S3,S6);//Shift Bit 0

						DNEWS0(S6,S2);
						MOV(S2,S6);//Shift Bit 1

						DNEWS0(S6,S1);
						MOV(S1,S6);//Shift Bit 2

						DNEWS0(S6,S0);
						MOV(S0,S6);//Shift Bit 3
					scamp5_kernel_end();
				}
			}

			int time_spent_on_dreg_shifting = dreg_shift_timer.get_usec();




		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//RETRIEVE 4-BIT IMAGE INTO AREG FROM SHIFTED DREG DATA

			{
				scamp5_in(C,-127);//Retrieved image will be placed into C
				scamp5_in(E,127);
				scamp5_kernel_begin();
					WHERE(S0);//Retrieve Bit 3 (128)
						add(C,C,E);
					ALL();

					divq(D,E);//D ~= 64
					WHERE(S1);//Retrieve Bit 2 (64)
						add(C,C,D);
					ALL();

					divq(E,D);//E ~= 32
					WHERE(S2);//Retrieve Bit 1 (32)
						add(C,C,E);
					ALL();

					divq(D,E);//D ~= 16
					WHERE(S3);//Retrieve Bit 0 (16)
						add(C,C,D);
					ALL();
				scamp5_kernel_end();
			}




		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SHIFT AREG IMAGE DATA TO COMPARE AGAINST SHIFTED DIGITAL DATA

			areg_shift_timer.reset();

			//SHIFT AREG DATA HORIZONTALLY
			{
				if(shift_x > 0)
				{
					for(int x = 0; x < shift_x; x++)
					{
						//KERNEL SHIFTS B ONE "PIXEL" RIGHT
						scamp5_kernel_begin();
							bus(NEWS,B);//NEWS = -B
							bus(B,XW);//B = -NEWS OF WEST NEIGHBOR
						scamp5_kernel_end();
					}
				}
				else
				{
					for(int x = 0; x < -shift_x; x++)
					{
						//KERNEL SHIFTS B ONE "PIXEL" LEFT
						scamp5_kernel_begin();
							bus(NEWS,B);//NEWS = -B
							bus(B,XE);//B = -NEWS OF EAST NEIGHBOR
						scamp5_kernel_end();
					}
				}
			}

			//NOW SHIFT AREG DATA VERTICALLY
			{
				if(shift_y > 0)
				{
					for(int y = 0; y < shift_y; y++)
					{
						//KERNEL SHIFTS B ONE "PIXEL" UP
						scamp5_kernel_begin();
							bus(NEWS,B);//NEWS = -B
							bus(B,XS);//B = -NEWS OF SOUTH NEIGHBOR
						scamp5_kernel_end();
					}
				}
				else
				{
					for(int y = 0; y < -shift_y; y++)
					{
						//KERNEL SHIFTS B ONE "PIXEL" DOWN
						scamp5_kernel_begin();
							bus(NEWS,B);//NEWS = -B
							bus(B,XN);//B = -NEWS OF NORTH NEIGHBOR
						scamp5_kernel_end();
					}
				}
			}
			int time_spent_on_areg_shifting = areg_shift_timer.get_usec();








		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT IMAGES

			output_timer.reset();
			output_4bit_image_via_DNEWS(B,display_00);
			output_4bit_image_via_DNEWS(C,display_01);


			scamp5_output_image(S3,display_bit0);
			scamp5_output_image(S2,display_bit1);
			scamp5_output_image(S1,display_bit2);
			scamp5_output_image(S0,display_bit3);
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
			vs_post_text("Shifting time AREG:%d , DREGx4 %d \n",time_spent_on_areg_shifting,time_spent_on_dreg_shifting);
    }

    return 0;
}


