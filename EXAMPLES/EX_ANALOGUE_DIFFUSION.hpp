#include <scamp5.hpp>
#include "../../s5d_m0_scamp5/src/debug_gui.hpp"

using namespace SCAMP5_PE;

void setup_blur_grid_in_S3(int offy, int offx, int gs_width, int gs_height);

int main(){

    // Initialization
    vs_init();

    // Setup Host GUI
    vs_gui_set_info(VS_M0_PROJECT_INFO_STRING);
    int display_size = 2;
    auto display_00 = vs_gui_add_display("00",0,0,display_size);
    auto display_01 = vs_gui_add_display("01",0,display_size,display_size);
    auto display_02 = vs_gui_add_display("02",0,display_size*2,display_size);

    auto display_10 = vs_gui_add_display("10",display_size,0,display_size);
    auto display_11 = vs_gui_add_display("11",display_size,display_size,display_size);

	int use_camera_img = 0;
	vs_gui_add_switch("use_camera_img: ",false,&use_camera_img);

	int iterations = 30;
	vs_gui_add_slider("iterations: ",1,50,iterations,&iterations);

	int x = 210;
	int y = 128;
	vs_gui_add_slider("x: ",0,255,x,&x);
	vs_gui_add_slider("y: ",0,255,y,&y);
	int w = 32;
	int h = 32;
	vs_gui_add_slider("w: ",1,64,w,&w);
	h = vs_gui_add_slider("h: ",3,64,h,&h);

	int x2 = 50;
	int y2 = 175;
	vs_gui_add_slider("x2: ",0,255,x2,&x2);
	vs_gui_add_slider("y2: ",0,255,y2,&y2);
	int w2 = 32;
	int h2 = 32;
	vs_gui_add_slider("w2: ",1,64,w2,&w2);
	h = vs_gui_add_slider("h2: ",3,64,h2,&h2);

	int blur_method = 1;
	vs_gui_add_switch("blur_method: ",blur_method == 1,&blur_method);

	int set_RS = 1;
	vs_gui_add_switch("set_RS: ",set_RS == 1,&set_RS);
	int set_RW = 1;
	vs_gui_add_switch("set_RW: ",set_RW == 1,&set_RW);

	int use_blur_grid = 0;
	vs_gui_add_switch("use_blur_grid: ",use_blur_grid == 1,&use_blur_grid);

	// in main after GUI setup section
	setup_voltage_configurator(false);

	vs_stopwatch timer;

    // Frame Loop
    while(1){
        vs_frame_loop_control();

		if(use_camera_img)
		{
			int gain = vs_gui_read_slider(VS_GUI_FRAME_GAIN);
			scamp5_get_image(A,F,gain);
		}
		else
		{
			//DRAW TEST IMAGE INTO REG A
			int block_size = 32;
			scamp5_kernel_begin();
				CLR(S6);
			scamp5_kernel_end();
			scamp5_draw_begin(S6);
				for(int y = 0 ; y < 256 ; y+=block_size)
				{
					for(int x = 0 ; x < 256 ; x+=block_size*2)
					{
						if(x < 64 || x >= 192 || y < 64 || y >= 192)
						{
							int x2 = x;
							if((y/block_size)%2 == 0)
							{
								x2+=block_size;
							}
							scamp5_draw_rect(y,x2,y+block_size,x2+block_size);
						}
					}
				}


				int rad = 48;
				int thickness = 4;
				for(int r = rad - thickness ; r <  rad + thickness; r++)
				{
					scamp5_draw_circle(127,127,r);
				}

				int tmp_size = 2;
				scamp5_draw_rect(127-tmp_size,127-tmp_size,127+tmp_size,127+tmp_size);

			scamp5_draw_end();

			scamp5_in(F,100);
			scamp5_in(A,-100);
			scamp5_kernel_begin();
				WHERE(S6);
					add(A,A,F);
					add(A,A,F);
				ALL();
				mov(F,A);
				add(A,A,F);
			scamp5_kernel_end();
		}


		if(!use_blur_grid)
		{
			//apply -1 to width and heights to blur within correctly sized rects
			scamp5_load_region(S5,y,x,y+h-1,x+w-1);
			scamp5_load_region(S6,y2,x2,y2+h2-1,x2+w2-1);
	    	scamp5_kernel_begin();
	    		OR(S3,S5,S6);
			scamp5_kernel_end();
		}
		else
		{
			//SETUP PATTERN TO BLUR MUTUALLY EXCLUSIVE GRID CELLS
			timer.reset();
			setup_blur_grid_in_S3(y%h, x%w,w,h);
			vs_post_text("%d \n", timer.get_usec());
		}


		if(set_RS)
		{
			scamp5_kernel_begin();
				MOV(RS,S3);
			scamp5_kernel_end();
		}
		if(set_RW)
		{
			scamp5_kernel_begin();
				MOV(RW,S3);
			scamp5_kernel_end();
		}

    	if(blur_method)
    	{
    	     scamp5_dynamic_kernel_begin();
				blur_repeat(D,A,iterations);
			scamp5_dynamic_kernel_end();
    	}
    	else
    	{
    	     scamp5_dynamic_kernel_begin();
				newsblur(D,A,iterations);
			scamp5_dynamic_kernel_end();
    	}



        if(vs_gui_is_on())
        {

            scamp5_output_image(RS,display_10);
		    scamp5_output_image(RW,display_11);

            scamp5_output_image(A,display_00);
            scamp5_output_image(D,display_01);
            scamp5_output_image(S4,display_02);

        }

    }

    return 0;
}

void setup_blur_grid_in_S3(int offy, int offx, int gs_width, int gs_height)
{
	//SETUP PATTERN TO BLUR MUTUALLY EXCLUSIVE GRID CELLS
	scamp5_kernel_begin();
		CLR(S4);
	scamp5_kernel_end();

	scamp5_load_region(S3,offy%gs_height,offx%gs_width,offy%gs_height+gs_height-2,offx%gs_width+gs_width-2);
	scamp5_kernel_begin();
		CLR(RE,RW,RN,RS);
		SET(RE);
		MOV(S5,S3);
	scamp5_kernel_end();

	for(int gx = 0; gx < 255/gs_width ; gx++)
	{
		scamp5_dynamic_kernel_begin();//CAN MAKE X6 FASTER BY REMOVING DYNAMIC
			for(int tmp_x = 0 ; tmp_x < gs_width ; tmp_x++)
			{
				DNEWS0(S4,S5);
				MOV(S5,S4);
			}
			MOV(S4,S3);
			OR(S3,S5,S4);
		scamp5_dynamic_kernel_end();
	}

	scamp5_kernel_begin();
		CLR(RE,RW,RN,RS);
		SET(RN);
		MOV(S5,S3);
	scamp5_kernel_end();

	for(int gy = 0; gy < 255/gs_height ; gy++)
	{
		scamp5_dynamic_kernel_begin();//CAN MAKE X6 FASTER BY REMOVING DYNAMIC
			for(int tmp_y = 0 ; tmp_y < gs_height ; tmp_y++)
			{
				DNEWS0(S4,S5);
				MOV(S5,S4);
			}
			MOV(S4,S3);
			OR(S3,S5,S4);
		scamp5_dynamic_kernel_end();
	}
}
