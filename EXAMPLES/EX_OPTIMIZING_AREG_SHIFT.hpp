#include <scamp5.hpp>

using namespace SCAMP5_PE;
void shift_F(int shiftx, int shifty);

int main()
{
    vs_init();

    int display_size = 2;
    auto display_00 = vs_gui_add_display("00",0,0,display_size);
    auto display_01 = vs_gui_add_display("01",0,display_size,display_size);

    int shiftx = 0;
    vs_gui_add_slider("shiftx",-64,64,shiftx,&shiftx);
    int shifty = 0;
    vs_gui_add_slider("shifty",-64,64,shifty,&shifty);

    vs_stopwatch timer;

    // Frame Loop
    while(1){
        vs_frame_loop_control();

        scamp5_kernel_begin();
        	get_image(A,E);
        	mov(F,A);
        scamp5_kernel_end();



        timer.reset();
        scamp5_shift(A,shiftx,shifty);
        int scamp5_shift_time = timer.get_usec();

        timer.reset();
        shift_F(shiftx,shifty);
        int my_shift_time = timer.get_usec();

        vs_post_text("scamp5_shift_time %d vs optimised shift %d us \n", scamp5_shift_time,my_shift_time);

		scamp5_output_image(A,display_00);
		scamp5_output_image(F,display_01);
    }

    return 0;
}

void shift_F(int shiftx, int shifty)
{
	if(shiftx > 0)
	{
		while(shiftx > 0)
		{
			if(shiftx % 8 == 0)
			{
				scamp5_kernel_begin();
					bus(XE,F);
					bus(E,XW);
					bus(XE,E);
					bus(F,XW);
					bus(XE,E);
					bus(F,XW);
					bus(XE,F);
					bus(F,XW);
				scamp5_kernel_end();
				shiftx-=8;
			}
			else
			{
				if(shiftx % 4 == 0)
				{
					scamp5_kernel_begin();
						bus(XE,F);
						bus(F,XW);
						bus(XE,F);
						bus(F,XW);
					scamp5_kernel_end();
					shiftx-=4;
				}
				else
				{
					if(shiftx % 2 == 0)
					{
						scamp5_kernel_begin();
							bus(XE,F);
							bus(F,XW);
						scamp5_kernel_end();
						shiftx-=2;
					}
					else
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XW);
						scamp5_kernel_end();
						shiftx--;
					}
				}
			}
		}
	}
	else
	{
		while(shiftx < 0)
		{

			if(shiftx % 8 == 0)
			{
				scamp5_kernel_begin();
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
					bus(NEWS,F);
					bus(F,XE);
				scamp5_kernel_end();
				shiftx+=8;
			}
			else
			{
				if(shiftx % 4 == 0)
				{
					scamp5_kernel_begin();
						bus(NEWS,F);
						bus(F,XE);
						bus(NEWS,F);
						bus(F,XE);
						bus(NEWS,F);
						bus(F,XE);
						bus(NEWS,F);
						bus(F,XE);
					scamp5_kernel_end();
					shiftx+=4;
				}
				else
				{
					if(shiftx % 2 == 0)
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XE);
							bus(NEWS,F);
							bus(F,XE);
						scamp5_kernel_end();
						shiftx+=2;
					}
					else
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XE);
						scamp5_kernel_end();
						shiftx++;
					}
				}
			}
		}
	}

	if(shifty > 0)
	{
		while(shifty > 0)
		{
			if(shifty % 8 == 0)
			{
				scamp5_kernel_begin();
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
					bus(NEWS,F);
					bus(F,XS);
				scamp5_kernel_end();
				shifty-=8;
			}
			else
			{
				if(shifty % 4 == 0)
				{
					scamp5_kernel_begin();
						bus(NEWS,F);
						bus(F,XS);
						bus(NEWS,F);
						bus(F,XS);
						bus(NEWS,F);
						bus(F,XS);
						bus(NEWS,F);
						bus(F,XS);
					scamp5_kernel_end();
					shifty-=4;
				}
				else
				{
					if(shifty % 2 == 0)
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XS);
							bus(NEWS,F);
							bus(F,XS);
						scamp5_kernel_end();
						shifty-=2;
					}
					else
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XS);
						scamp5_kernel_end();
						shifty--;
					}
				}
			}
		}
	}
	else
	{
		while(shifty < 0)
		{
			if(shifty % 8 == 0)
			{
				scamp5_kernel_begin();
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
					bus(NEWS,F);
					bus(F,XN);
				scamp5_kernel_end();
				shifty+=8;
			}
			else
			{
				if(shifty % 4 == 0)
				{
					scamp5_kernel_begin();
						bus(NEWS,F);
						bus(F,XN);
						bus(NEWS,F);
						bus(F,XN);
						bus(NEWS,F);
						bus(F,XN);
						bus(NEWS,F);
						bus(F,XN);
					scamp5_kernel_end();
					shifty+=4;
				}
				else
				{
					if(shifty % 2 == 0)
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XN);
							bus(NEWS,F);
							bus(F,XN);
						scamp5_kernel_end();
						shifty+=2;
					}
					else
					{
						scamp5_kernel_begin();
							bus(NEWS,F);
							bus(F,XN);
						scamp5_kernel_end();
						shifty++;
					}
				}
			}
		}
	}
}
