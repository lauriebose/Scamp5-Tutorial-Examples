#include <scamp5.hpp>
#include <vector>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;

int main()
{
    vs_init();

    //See link...
    //https://scamp.gitlab.io/scamp5d_doc/_p_a_g_e__g_u_i_d_e__g_u_i.html#sec_host_gui_3
	VS_GUI_DISPLAY_STYLE(style_plot,R"JSON(
			{
				"plot_palette": "plot_4",
				"plot_palette_groups": 4
			}
			)JSON");

    int display_size = 2;

    //Add a display that will contain the bar chart
    auto plot_display = vs_gui_add_display("",0,0,display_size,style_plot);

    //Setup the bar chart within the display using "vs_gui_set_barplot"
    const int plot_min_value = 0;
    const int plot_max_value = 10000;
    const int plot_bar_width = 64;
    const int plot_bars = 4;
    vs_gui_set_barplot(plot_display,plot_min_value,plot_max_value,plot_bar_width);

    //Add some variables and sliders to demonstrate plotting
    int value_1 = 3000;
    int value_2 = 6000;
    int value_3 = 8000;
    int value_4 = 0;
    vs_handle slider_value1 = vs_gui_add_slider("Value 1",0,10000,value_1,&value_1);
    vs_handle slider_value2 =vs_gui_add_slider("Value 2",0,10000,value_2,&value_2);
    vs_handle slider_value3 =vs_gui_add_slider("Value 3",-0,10000,value_3,&value_3);
    vs_handle slider_value4 =vs_gui_add_slider("Value 4",0,10000,value_4,&value_4);

    // Frame Loop
    while(1)
    {
		frame_timer.reset();//reset frame_timer
		vs_disable_frame_trigger();//Disable frame trigger to run as fast as possible
		vs_frame_loop_control();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//UPDATE "value_4" TO MAKE OUR PLOT MORE INTERESTING
			int times_looped = vs_loop_counter_get();//Get the number of times this while loop has been executed
			int tmp_value = times_looped%plot_max_value;
			vs_gui_move_slider(slider_value4, tmp_value);//Move the slider tied to "value_4"

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//SEND DATA TO PLOT TO THE DISPLAY OF THE BAR CHART

			int32_t plot_data[4]; //Create an array of data which will be sent to the display of the bar chart

			//Fill in the array with our test values
			plot_data[0] = value_1;
			plot_data[1] = value_2;
			plot_data[2] = value_3;
			plot_data[3] = value_4;

			vs_post_set_channel(plot_display);//Select the target to which "vs_post" will send data to. In this case the display of the bar chart
			vs_post_int32(plot_data,1,plot_bars);//Post the array of data, the bar chart within the display should update accordingly

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			vs_post_text("frame time %d microseconds, potential FPS ~%d \n",frame_time_microseconds,max_possible_frame_rate); //display this values on host
    }

    return 0;
}

