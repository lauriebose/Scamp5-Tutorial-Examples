#include <scamp5.hpp>
#include <vector>

#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch sum_timer;

const int record_length = 100;


std::pair<int, int> computeMeanAndVariance(const std::vector<uint16_t>& data)
{
    int sum = 0;
    for (uint8_t value : data)
    {
        sum += value;
    }
    int mean = sum / data.size();

    int sqDiffSum = 0;
    for (uint8_t value : data)
    {
        int diff = value - mean;
        sqDiffSum += diff * diff;
    }

    int variance = sqDiffSum / (data.size() - 1);

    return {mean, variance};
}

int main()
{
    vs_init();

	VS_GUI_DISPLAY_STYLE(style_plot,R"JSON(
			{
				"plot_palette": "plot_4",
				"plot_palette_groups": 4
			}
			)JSON");

    int display_size = 2;
    auto display_00 = vs_gui_add_display("Image to Sum",0,0,display_size);

    auto display_plot = vs_gui_add_display("Red:Sum64, Yellow:Sum16, Green:Sum_fast",0,display_size,display_size,style_plot);
    vs_gui_set_scope(display_plot,0,100,255);

    int use_test_input = 0;
    vs_gui_add_switch("use test input", use_test_input == 1, &use_test_input);

    int use_binary_test_input = 0;
    vs_gui_add_switch("binary test input", use_binary_test_input == 1, &use_binary_test_input);

    int input_value = 0;
    vs_gui_add_slider("input_value",-127,127,input_value,&input_value);

    int record_cntr = 0;
    std::vector<uint16_t> sum64_record(record_length);
    std::vector<uint16_t> sum16_record(record_length);
    std::vector<uint16_t> sum_fast_record(record_length);

	int sum64_max, sum64_min;
	int sum16_max, sum16_min;
	int sum_fast_max, sum_fast_min;

	//Compute the maximum & minimum values for each type of summation
	{
		scamp5_in(F,-127);
		scamp5_kernel_begin();
			mov(A,F);
			add(A,A,F);
			add(A,A,F);
		scamp5_kernel_end();
		sum64_min = scamp5_global_sum_64(A) + 1;//+1s to avoid possibility of divide by 0 later
		sum16_min = scamp5_global_sum_16(A) + 1;
		sum_fast_min = scamp5_global_sum_fast(A) + 1;

		scamp5_in(F,127);
		scamp5_kernel_begin();
			mov(A,F);
			add(A,A,F);
			add(A,A,F);
		scamp5_kernel_end();
		sum64_max = scamp5_global_sum_64(A);
		sum16_max = scamp5_global_sum_16(A);
		sum_fast_max = scamp5_global_sum_fast(A);
	}

    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//capture new image into AREG A and create copy in B

			if(use_test_input)
			{
				if(!use_binary_test_input)
				{
					scamp5_in(A,input_value);
				}
				else
				{
					scamp5_in(E,127);
					scamp5_in(F,-127);
					scamp5_load_rect(S6,0,0,255,input_value+127);
					scamp5_kernel_begin();
						mov(A,F);
						WHERE(S6);
							mov(A,E);
						ALL();
					scamp5_kernel_end();
				}
			}
			else
			{
				scamp5_kernel_begin();
					get_image(A,E);
					mov(B,A);
				scamp5_kernel_end();
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM VERTICAL HALF SCALING


			sum_timer.reset();
			int16_t sum64_result = scamp5_global_sum_64(A);
			int sum64_time = sum_timer.get_usec();

			sum_timer.reset();
			int16_t sum16_result = scamp5_global_sum_16(A);
			int sum16_time = sum_timer.get_usec();

			sum_timer.reset();
			int16_t sum_fast_result = scamp5_global_sum_fast(A);
			int sum_fast_time = sum_timer.get_usec();

			int sum64_result_normalised = (100*sum64_result)/(sum64_max - sum64_min);
			int sum16_result_normalised = (100*sum16_result)/(sum16_max - sum16_min);
			int sum_fast_result_normalised = (100*sum_fast_result)/(sum_fast_max - sum_fast_min);

			sum64_record[record_cntr] = sum64_result_normalised;
			sum16_record[record_cntr] = sum16_result_normalised;
			sum_fast_record[record_cntr] = sum_fast_result_normalised;
			record_cntr = (record_cntr+1)%record_length;

			auto [sum64_mean, sum64_var] = computeMeanAndVariance(sum64_record);
			auto [sum16_mean, sum16_var] = computeMeanAndVariance(sum16_record);
			auto [sum_fast_mean, sum_fast_var] = computeMeanAndVariance(sum_fast_record);

			vs_post_text("sum64 - val %d, normalised val %d, mean %d, variance %d, time %d \n",sum64_result, sum64_result_normalised,sum64_mean,sum64_var, sum64_time);
			vs_post_text("sum16 - val %d, normalised val %d , mean %d, variance %d, time %d \n",sum16_result,sum16_result_normalised,sum16_mean,sum16_var,sum16_time);
			vs_post_text("sum_fast - val %d, normalised val %d , mean %d, variance %d, time %d \n",sum_fast_result,sum_fast_result_normalised,sum_fast_mean,sum_fast_var,sum_fast_time);

		    //Post data for plot of execution times of each method
			int32_t plot_data[4];
			plot_data[0] = sum64_result_normalised;
			plot_data[1] = sum16_result_normalised;
			plot_data[2] = sum_fast_result_normalised;
			vs_post_set_channel(display_plot);
			vs_post_int32(plot_data,1,3);


		   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//OUTPUT IMAGES

				output_timer.reset();

				scamp5_output_image(A,display_00);
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


