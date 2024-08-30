
#define EX_KERNEL_COST -1
#define EX_AREG_DECAY -2
#define EX_AREG_SATURATION -3
#define EX_EVENT_READOUT -4
#define EX_AREG_DEGRADATION -5
#define EX_DREG_LOAD_PATTERN -6
#define EX_RF_AND_RM -7
#define EX_GLOBAL_OR -8
#define EX_SHAPE_EXTRACTION -9
#define EX_DREG_BOUNDING_BOX -10
#define EX_DREG_FLOODING -11

#define EX_INTERLEAVED_DATA -15
#define EX_ADC_DAC -16
#define EX_DREG_SHIFTING1 -17
#define EX_DREG_SHIFTING2 -18
#define EX_AREG_QUANTIZED_STORAGE - 19

#define EX_IMAGE_CAPTURE 0
#define EX_IMAGE_CAPTURE_AND_AREG 1
#define	EX_DREG_BASICS 2
#define EX_FLAG 3
#define EX_IMAGE_THRESHOLDING 4
#define	EX_AREG_NEWS 5
#define	EX_SIMPLE_EDGE_DETECTION 6
#define	EX_DNEWS 7
#define EX_DREG_SHIFTING 8
#define EX_DREG_EXPAND_AND_ERODE 9
#define EX_HALF_SCALING 10

#define selected_algo -19


#if selected_algo == 999
#elif selected_algo == EX_IMAGE_CAPTURE
	#include "EX00_IMAGE_CAPTURE.hpp"
#elif selected_algo == EX_IMAGE_CAPTURE_AND_AREG
	#include "EX01_IMAGE_CAPTURE_AND_AREG.hpp"
#elif selected_algo == EX_DREG_BASICS
	#include "EX02_DREG_BASICS.hpp"
#elif selected_algo == EX_FLAG
	#include "EX03_FLAG.hpp"
#elif selected_algo == EX_IMAGE_THRESHOLDING
	#include "EX04_IMAGE_THRESHOLDING.hpp"
#elif selected_algo == EX_AREG_NEWS
	#include "EX05_AREG_NEWS.hpp"
#elif selected_algo == EX_SIMPLE_EDGE_DETECTION
	#include "EX06_SIMPLE_EDGE_DETECTION.hpp"
#elif selected_algo == EX_DNEWS
	#include "EX07_DNEWS.hpp"
#elif selected_algo == EX_DREG_EXPAND_AND_ERODE
	#include "EX09_DREG_EXPAND_AND_ERODE.hpp"
#elif selected_algo == EX_HALF_SCALING
	#include "EX10_HALF_SCALING.hpp"
#elif selected_algo == EX_INTERLEAVED_DATA
	#include "EX_INTERLEAVED_DATA.hpp"
#elif selected_algo == EX_ADC_DAC
	#include "EX_ADC_DAC.hpp"
#elif selected_algo == EX_DREG_SHIFTING1
	#include "EX_DREG_SHIFTING1.hpp"
#elif selected_algo == EX_DREG_SHIFTING2
	#include "EX_DREG_SHIFTING2.hpp"

#elif selected_algo == EX_EVENT_READOUT
	#include "EX_EVENT_READOUT.hpp"
#elif selected_algo == EX_DREG_FLOODING
	#include "EX_DREG_FLOODING.hpp"
#elif selected_algo == EX_AREG_DECAY
	#include "EX_AREG_DECAY.hpp"
#elif selected_algo == EX_KERNEL_COST
	#include "EX_KERNEL_COST.hpp"
#elif selected_algo == EX_AREG_SATURATION
	#include "EX_AREG_SATURATION.hpp"
#elif selected_algo == EX_AREG_DEGRADATION
	#include "EX_AREG_DEGRADATION.hpp"
#elif selected_algo == EX_DREG_LOAD_PATTERN
	#include "EX_DREG_LOAD_PATTERN.hpp"
#elif selected_algo == EX_RF_AND_RM
	#include "EX_RF_AND_RM.hpp"
#elif selected_algo == EX_GLOBAL_OR
	#include "EX_GLOBAL_OR.hpp"
#elif selected_algo == EX_SHAPE_EXTRACTION
	#include "EX_SHAPE_EXTRACTION.hpp"
#elif selected_algo == EX_DREG_BOUNDING_BOX
	#include "EX_DREG_BOUNDING_BOX.hpp"
#elif selected_algo == EX_AREG_QUANTIZED_STORAGE
	#include "EX_AREG_QUANTIZED_STORAGE.hpp"

#endif

