#include <scamp5.hpp>
#include <vector>
#include "MISC/MISC_FUNCS.hpp"
using namespace SCAMP5_PE;

vs_stopwatch frame_timer;
vs_stopwatch output_timer;
vs_stopwatch sum_timer;

//Variables used for calculating statistics for the difference global summation functions
int sum64_max, sum64_min;
int sum16_max, sum16_min;
int sum_fast_max, sum_fast_min;

const int record_length = 100;
int record_cntr = 0;
std::vector<uint16_t> sum64_record(record_length);
std::vector<uint16_t> sum16_record(record_length);
std::vector<uint16_t> sum_fast_record(record_length);

//Function used to compute moving average & variance of the output from each of the global summation functions
std::pair<int, int> calculate_mean_and_variance_of_data_array(const std::vector<uint16_t>& data);

void compute_maximum_and_minimum_of_global_sum_functions();

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

    //Add display to plot the results from each global summation function
    auto display_plot = vs_gui_add_display("Red:Sum64, Yellow:Sum16, Green:Sum_fast",0,display_size,display_size,style_plot);
    const int plot_min = 0;
    const int plot_max = 100;
    const int plot_time_frame = 256;
    vs_gui_set_scope(display_plot,plot_min,plot_max,plot_time_frame);

    int use_test_input = 0;
    vs_gui_add_switch("use test input", use_test_input == 1, &use_test_input);

    int use_binary_test_input = 0;
    vs_gui_add_switch("binary test input", use_binary_test_input == 1, &use_binary_test_input);

    int input_value = 0;
    vs_gui_add_slider("input_value",-127,127,input_value,&input_value);

    //Compute the range of possible results from each summation function
    compute_maximum_and_minimum_of_global_sum_functions();

    // Frame Loop
    while(1)
    {
        frame_timer.reset();//reset frame_timer

       	vs_disable_frame_trigger();
        vs_frame_loop_control();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//CAPTURE OR CREATE THE ANALOGUE DATA TO TEST GLOBAL SUMMATION UPON

			if(!use_test_input)
			{
				//Capture an Image
				scamp5_kernel_begin();
					get_image(A,E);
					mov(B,A);
				scamp5_kernel_end();
			}
			else
			{
				if(!use_binary_test_input)
				{
					//Create an image with all pixels of a specific value
					scamp5_in(A,input_value);
				}
				else
				{
					//Create an image with a certain proportion of pixels the maximum value (and other pixels the minimum value)
					scamp5_in(E,127);
					scamp5_in(F,-127);
					scamp5_load_rect(S6,0,0,255,input_value+127);//Load a rectangle occupying of proportion of the register plane / image

					//In PEs outside the rectangle place -127, PEs inside the rectangle place +127
					scamp5_kernel_begin();
						mov(A,F);
						WHERE(S6);
							mov(A,E);
						ALL();
					scamp5_kernel_end();
				}
			}

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //PERFORM GLOBAL SUMMATION USING EACH FUNCTION

			sum_timer.reset();
			int16_t sum64_result = scamp5_global_sum_64(A);
			int sum64_time = sum_timer.get_usec();

			sum_timer.reset();
			int16_t sum16_result = scamp5_global_sum_16(A);
			int sum16_time = sum_timer.get_usec();

			sum_timer.reset();
			int16_t sum_fast_result = scamp5_global_sum_fast(A);
			int sum_fast_time = sum_timer.get_usec();

	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   //COMPUTE AND RECORD THE NORMALISED RESULTS FOP EACH FUNCTION

			//Calculate normalised results, places each value in the range 0-100
			int sum64_result_normalised = (100*sum64_result)/(sum64_max - sum64_min);
			int sum16_result_normalised = (100*sum16_result)/(sum16_max - sum16_min);
			int sum_fast_result_normalised = (100*sum_fast_result)/(sum_fast_max - sum_fast_min);

			//Record these results for computation of the moving average and variance
			sum64_record[record_cntr] = sum64_result_normalised;
			sum16_record[record_cntr] = sum16_result_normalised;
			sum_fast_record[record_cntr] = sum_fast_result_normalised;
			record_cntr = (record_cntr+1)%record_length;//Update the cntr to the next location in the array

			auto [sum64_mean, sum64_var] = calculate_mean_and_variance_of_data_array(sum64_record);
			auto [sum16_mean, sum16_var] = calculate_mean_and_variance_of_data_array(sum16_record);
			auto [sum_fast_mean, sum_fast_var] = calculate_mean_and_variance_of_data_array(sum_fast_record);

	   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	   //OUTPUT

			vs_post_text("sum64 - val %d, normalised val %d, mean %d, variance %d, time %d \n",sum64_result, sum64_result_normalised,sum64_mean,sum64_var, sum64_time);
			vs_post_text("sum16 - val %d, normalised val %d , mean %d, variance %d, time %d \n",sum16_result,sum16_result_normalised,sum16_mean,sum16_var,sum16_time);
			vs_post_text("sum_fast - val %d, normalised val %d , mean %d, variance %d, time %d \n",sum_fast_result,sum_fast_result_normalised,sum_fast_mean,sum_fast_var,sum_fast_time);

			//Create an array of data with the latest values to plot
			int32_t plot_data[3];
			plot_data[0] = sum64_result_normalised;
			plot_data[1] = sum16_result_normalised;
			plot_data[2] = sum_fast_result_normalised;
			vs_post_set_channel(display_plot);//Set target to "post" data to, the display setup for plotting
			vs_post_int32(plot_data,1,3);//Post the array of data, this should now be plotted in the display

			output_timer.reset();
			{
				scamp5_output_image(A,display_00);
			}
			int image_output_time_microseconds = output_timer.get_usec();//get the time taken for image output

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//OUTPUT TEXT INFO

			int frame_time_microseconds = frame_timer.get_usec(); //get the time taken this frame
			int max_possible_frame_rate = 1000000/frame_time_microseconds; //calculate the possible max FPS
			int image_output_time_percentage = (image_output_time_microseconds*100)/frame_time_microseconds; //calculate the % of frame time which is used for image output
			vs_post_text("frame time %d microseconds(%%%d image output), potential FPS ~%d \n",frame_time_microseconds,image_output_time_percentage,max_possible_frame_rate); //display this values on host
    }

    return 0;
}

std::pair<int, int> calculate_mean_and_variance_of_data_array(const std::vector<uint16_t>& data)
{
	//Calculate sum of all data & mean
    int sum = 0;
    for (uint8_t value : data)
    {
        sum += value;
    }
    int mean = sum / data.size();

    //Calculate sum of squared differences from the mean
    int sum_of_sqrd_differences = 0;
    for (uint8_t value : data)
    {
        int diff = value - mean;
        sum_of_sqrd_differences += diff * diff;
    }
    int variance = sum_of_sqrd_differences / data.size();

    //return both mean and variance as a pair
    return {mean, variance};
}

//Compute the maximum & minimum values for each type of summation
void compute_maximum_and_minimum_of_global_sum_functions()
{
	//Make register plane A as negative as possible
	scamp5_in(F,-127);
	scamp5_kernel_begin();
		mov(A,F);
		add(A,A,F);
	scamp5_kernel_end();

	sum64_min = scamp5_global_sum_64(A) + 1;//+1s to avoid possibility of divide by 0 later
	sum16_min = scamp5_global_sum_16(A) + 1;
	sum_fast_min = scamp5_global_sum_fast(A) + 1;

	//Make register plane A as positive  as possible
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


