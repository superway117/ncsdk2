///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Application configuration Leon header
///

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include "fatalExtension.h"
#include "mv_types.h"

// PLL desired frequency
#define PLL_DESIRED_FREQ_KHZ   600000

// Default start up clock
#define DEFAULT_OSC_CLOCK_KHZ   24000

#define L2CACHE_NORMAL_MODE     (0x6)  // In this mode the L2Cache acts as a cache for the DRAM
#define L2CACHE_CFG         (L2CACHE_NORMAL_MODE)

#define CMX_CONFIG_SLICE_7_0       (0x11111111)
#define CMX_CONFIG_SLICE_15_8      (0x11111111)

#endif // _APP_CONFIG_H_
