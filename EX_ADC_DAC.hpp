#include <scamp5.hpp>
using namespace SCAMP5_PE;
#include "MISC/MISC_FUNCS.hpp"

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    const int display_size = 2;
    vs_handle display_00 = vs_gui_add_display("Image",0,0,display_size);
    vs_handle display_01 = vs_gui_add_display("Image Retrieved from DREG",0,display_size,display_size);

    vs_handle display_bit0 = vs_gui_add_display("DREG ADC Bit 0",0,display_size*2,1);
    vs_handle display_bit1 = vs_gui_add_display("DREG ADC Bit 1",0,display_size*2+1,1);
    vs_handle display_bit2 = vs_gui_add_display("DREG ADC Bit 2",1,display_size*2,1);
    vs_handle display_bit3 = vs_gui_add_display("DREG ADC Bit 3",1,display_size*2+1,1);

    int update_stored_image = 1;
    vs_gui_add_switch("update_stored_image",true,&update_stored_image);

    while(1)
    {
    	frame_timer.reset();

    	vs_disable_frame_trigger();
        vs_frame_loop_control();

    	scamp5_kernel_begin();
			get_image(A,F);
		scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//STORE COPY OF CAPTURED IMAGE IN ANOTHER AREG

			if(update_stored_image == 1)
			{
				scamp5_kernel_begin();
					mov(B,A);
				scamp5_kernel_end();
			}



        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM ADC (ANALOGUE TO DIGITAL CONVERSION), CONVERT CAPTURED IMAGE FROM AREG, TO 4BIT IMAGE IN 4 DREG
		//S0 = Bit 3 (Represents value ~128)
		//S1 = Bit 2 (Represents value ~64)
		//S2 = Bit 1 (Represents value ~32)
		//S3 = Bit 0 (Represents value ~16)
		//Take AREG values to range from -127 to 127

        	if(update_stored_image == 1)
        	{
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
        	}




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
    }

    return 0;
}


