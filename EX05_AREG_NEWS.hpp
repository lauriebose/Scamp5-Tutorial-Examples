#include <scamp5.hpp>
#include "MISC/OUTPUT_AREG_BITSTACK.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch areg_shift_timer;

int main()
{
    vs_init();

    int display_size = 2;
    auto display_00 = vs_gui_add_display("Captured Image, AREG A",0,0,display_size);
    auto display_01 = vs_gui_add_display("Shifted Imag, AREG B",0,display_size,display_size);
    auto display_02 = vs_gui_add_display("Captured Image - Shifted Image, AREG C = A - B",0,2*display_size,display_size);

    int shift_x = 40;
    vs_gui_add_slider("shift x",-128,128,shift_x,&shift_x);
    int shift_y = 10;
    vs_gui_add_slider("shift y",-128,128,shift_y,&shift_y);


    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//capture new image into AREG A and create copy in B

			scamp5_kernel_begin();
				get_image(A,E);
				mov(B,A);
			scamp5_kernel_end();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //SHIFT AREG HORIZONTALLY

			areg_shift_timer.reset();

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

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//NOW SHIFT AREG VERTICALLY
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

			int time_spent_on_shifting = areg_shift_timer.get_usec();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //SUBTRACT SHIFTED IMAGE FROM ORIGINAL
			scamp5_kernel_begin();
				sub(C,A,B);
			scamp5_kernel_end();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT REGISTERS AS IMAGES

        	output_timer.reset();
        	output_4bit_image_via_DNEWS(A,display_00);//captured image
        	output_4bit_image_via_DNEWS(B,display_01);//shifted image
        	output_4bit_image_via_DNEWS(C,display_02);//captured image - shifted image
			int output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

	        vs_post_text("time spent on shifting %d microseconds\n",time_spent_on_shifting);

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}

