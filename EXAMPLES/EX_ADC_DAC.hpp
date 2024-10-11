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

            				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            				// Store Bit 3 to S0
            				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
					where(D);	// Make FLAG to 1 for any pixel which has value > 0, otherwise 0
							// PEs whose working image pixel is > 0
							// These PEs must have the 128 (Bit 3) DREG set
						MOV(S0,FLAG);	// Transfer the pixels which the value larger than 0 to S0 (R5) (Binarize) [S0 = 0 ~ 127]
							// Update Bit 3

						NOT(RF,FLAG);	// Invert FLAG to RF
					WHERE(RF);
						// Make FLAG to 1 for any pixel which has value < 0, otherwise 0
                          			// Note: When the FLAG -> 1 for one pixel, the agaloge register for that pixel would not work (MOV, etc.)
						// In PEs whose working image pixel is < 0, add 127
						// Can then use where() in later steps to to get the remaining bits for these PEs
						add(D,D,F);	// D = D + 127 for the pixels which have value smaller than 0 (For D, All pixels go to 0 - 128)
                            					// Make all pixels with negtive values become positive values, and ignore any pixels with positive values
							// For analogue register D, the value of pixel larger than 0 will be zero, otherwise turn to the positive value
							// E.g.
      								// -120 -> 7;   Y
      								// -100 -> 27;  Y
    							  	// -85 -> 42;   Y
      								// -50 -> 77;   Y
      								// -10 -> 117;  Y
      								// -1 -> 126;   Y
      								// 1 -> 0;      N
      								// 120 -> 0;    N
					all();	// All FLAG -> 1

				        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            				// Store Bit 2 to S1
            				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
					divq(E,F);	// E = 127 / 2 ~= 64
					mov(F,E);	// F ~= 64
					sub(E,D,F);	// E = D - 64. 
                           				// E.g. 
                                				// 7 - 64 = -57;
                                				// 27 - 64 = -37;
                                				// 42 - 64 = -22;
                                				// 77 - 64 = 13
                                				// 117 - 64 = 53; 
                                				// 126 - 64 = 62; 
                                				// 0 - 64 = -64; 
                                				// 0 - 64 = -64;
					where(E);	// Make FLAG to 1 for any pixel which has value -64 ~ 0, otherwise 0
						mov(D,E);	// Move the [pixels <- (which the value between -64 ~ 0)] to D, and COMBINE with other negtive pixels
			            				// E.g. 
                            						// -120 -> 7 -> 7;      N
                            						// -100 -> 27 -> 27;    N
                            						// -85 -> 42 -> 42;     N
                            						// -50 -> 77 -> 13;     Y
                            						// -10 -> 117 -> 53;    Y
                            						// -1 -> 126 -> 62;     Y
                            						// 1 -> 0 -> 0;         N
                            						// 120 -> 0 -> 0;       N
                        					// In D, All pixel values limit to 0 ~ 64
			
						MOV(S1,FLAG);	// Transfer the pixels which the value between -64 ~ 0 to S1 (Binarize) [S1 = (-64 ~ 0)]
								// Update Bit 2
					all();

					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Store Bit 1 to S2
					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
					divq(E,F);	// E = 64 / 2 ~= 32
					mov(F,E);	// F ~= 32
					sub(E,D,F);	// E = D - 32. 
                          				// E.g. 
                            					// 7 - 32 = -25;
                            					// 27 - 32 = -5; 
                            					// 42 - 32 = 10;
                            					// 12 - 32 = -20;
                            					// 53 - 32 = 21; 
                            					// 62 - 32 = 30; 
                           					// 0 - 32 = -32; 
                            					// 0 - 32 = -32
					where(E);	// Make FLAG to 1 for any pixel which has value (-96 ~ -64) AND (-32 ~ 0), otherwise 0
			
						mov(D,E);	// Move the [pixels <- (which the value between (-96 ~ -64) AND (-32 ~ 0)] to D (For D, All pixels go to 0 - 32)
			           				// E.g. 
                            						// -120 -> 7 -> 7 -> 7;     N
                            						// -100 -> 27 -> 27 -> 27;  N
                            						// -85 -> 42 -> 42 -> 10;   Y
                            						// -50 -> 77 -> 13 -> 13;   N
                            						// -10 -> 117 -> 53 -> 21;  Y
                            						// -1 -> 126 -> 62 -> 30;   Y
                            						// 1 -> 0 -> 0 -> 0;        N
                            						// 120 -> 0 -> 0 -> 0       N
						MOV(S2,FLAG);	// Transfer the pixels which the value (-96 ~ -64) AND (-32 ~ 0) to S2 (Binarize) [S2 = (-96 ~ -64) OR (-32 ~ 0)]
								// Update Bit 1
					all();

					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            				// Store Bit 0 to S3
            				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
					divq(E,F);	// E = 32 / 2 ~= 16
					mov(F,E);	// F ~= 16
					sub(E,D,F);	// E = D - 16. 
			              			// E.g. 
                            					// 7 - 16 = -9;
                            					// 27 - 16 = 11;
                            					// 10 - 16 = -6; 
                            					// 13 - 16 = -3; 
                            					// 21 - 16 = 5; 
                            					// 30 - 16 = 14;
                            					// 0 - 16 = -16;
                            					// 0 - 16 = -16;
					where(E);	// Make FLAG to 1 for any pixel which has value (-112 ~ -96) AND (-16 ~ 0), otherwise 0
						mov(D,E);	// Move the [pixels <- (which the value (-112 ~ -96) AND (-16 ~ 0)] to D (For D, All pixels go to 0 - 16)
                       						// E.g. 
                            						// -120 -> 7 -> 7 -> 7 -> 7;        N
                            						// -100 -> 27 -> 27 -> 27 -> 11;    Y
                            						// -85 -> 42 -> 42 -> 10 -> 10;     N
                            						// -50 -> 77 -> 13 -> 13 -> 13;     N
                            						// -10 -> 117 -> 53 -> 21 -> 5;     Y
                            						// -1 -> 126 -> 62 -> 30 -> 14;     Y
                            						// 1 -> 0 -> 0 -> 0 -> 0;           N
                            						// 120 -> 0 -> 0 -> 0 -> 0;         N
						MOV(S3,FLAG);	// Transfer the pixels which the value (-112 ~ -96) AND (-16 ~ 0) to S2 (Binarize) [S2 = (-112 ~ -96) OR (-16 ~ 0)]
								// Update Bit 0
					all();
				scamp5_kernel_end();
        	}




		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//RETRIEVE 4-BIT IMAGE INTO AREG FROM SHIFTED DREG DATA

			{
				scamp5_in(C,-127);	// Turn analogue registers in C to -127
                                			// Retrieved image will be placed into C
				scamp5_in(E,127);	// Turn analogue registers in E to 127
				scamp5_kernel_begin();
					WHERE(S0);	// Set FLAG to 1 when the pixels in S0 turn to 1
                        				// Retrieve Bit 3 (Any pixels with value larger than 0 in original image)
						add(C,C,E);	// C = C + E = -127 + 127 = 0 for the "1" pixels in S0
                            					// States of C: [2]
                                					// -127 (-127 ~ 0);
                                					// 0    (0 ~ 127);
					ALL();	// All FLAG -> 1

					divq(D,E);	// D ~= 64
					WHERE(S1);	// Retrieve Bit 2 (Any pixels with value (-64 ~ 0) in original image)
						add(C,C,D);	// C = C + D = -127 + 64 = -63 for the "1" pixels in S1
                            					// States of C: [3]
                                					// -127 (-127 ~ -64)
                                					// -64  (-64 ~ 0)
                                					// 0    (0 ~ 127)
					ALL();	// All FLAG -> 1

					divq(E,D);	// E ~= 32
					WHERE(S2);	// Retrieve Bit 1 (Any pixels with value (-96 ~ -64) AND (-32 ~ 0) in original image)
						add(C,C,E);	// C = C + E = -127 + 32 = -95 for the pixels with value (-96 ~ -64) in original image
                            					// C = C + E = -63 + 32 = -31 for the pixels with value (-32 ~ 0) in original image
                            					// States of C: [5]
                                					// -127 (-127 ~ -96)
                                					// -95  (-96 ~ -64)
                                					// -64  (-64 ~ -32)
                                					// -31  (-32 ~ 0)
                                					// 0    (0 ~ 127)
					ALL();	// All FLAG -> 1

					divq(D,E);	// D ~= 16
					WHERE(S3);	// Retrieve Bit 0 (Any pixels with value (-112 ~ -96) AND (-16 ~ 0) in original image)
						add(C,C,D);	// C = C + D = -127 + 16 = -111 for the pixels with value (-112 ~ -96) in original image
                            					// C = C + D = -31 + 16 = -15 for the pixels with value (-16 ~ 0) in original image
                            					// States of C: [7]
                                					// -127 (-127 ~ -112)
                                					// -111 (-112 ~ -96)
                                					// -95  (-96 ~ -64)
                                					// -64  (-64 ~ -32)
                                					// -31  (-32 ~ -16)
                                					// -16  (-16 ~ 0)
                                					// 0    (0 ~ 127)
					ALL();	// All FLAG -> 1
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


