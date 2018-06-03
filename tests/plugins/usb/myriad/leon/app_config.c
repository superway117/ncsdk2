///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief    Application configuration Leon file
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "app_config.h"
#include <DrvLeonL2C.h>
#include <OsDrvCpr.h>
#include <stdlib.h>
#include "OsDrvInit.h"
#include <bsp.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
CmxRamLayoutCfgType __attribute__((section(".cmx.ctrl"))) __cmx_config = {CMX_CONFIG_SLICE_7_0, CMX_CONFIG_SLICE_15_8};

// 4: Static Local Data
// ----------------------------------------------------------------------------

// System Clock configuration on start-up
BSP_SET_CLOCK(DEFAULT_OSC_CLOCK_KHZ,
              PLL_DESIRED_FREQ_KHZ,
              1,
              1,
              DEFAULT_RTEMS_CSS_LOS_CLOCKS,
              DEFAULT_RTEMS_MSS_LRT_CLOCKS,
              DEFAULT_UPA_CLOCKS,
              0,
              0);

// Program caches behaviour
BSP_SET_L2C_CONFIG(1, DEFAULT_RTEMS_L2C_REPLACEMENT_POLICY, DEFAULT_RTEMS_L2C_LOCKED_WAYS,DEFAULT_RTEMS_L2C_MODE, 0, 0);

static OsDrvCprDeviceConfig devices[]= {
    {OS_DRV_CPR_DEV_CSS_USB,   OS_DRV_CPR_DEV_ENABLE},
    OS_DRV_CPR_DEV_ARRAY_TERMINATOR
};

// parameters to be passed to CPR platform init
static OsDrvCprAuxClockConfig auxClkConfig[] =
{
   {OS_DRV_CPR_DEV_CSS_AUX_USB_PHY_REF_ALT, OS_DRV_CPR_CLK_PLL0, 1, 25},
   {OS_DRV_CPR_DEV_CSS_AUX_USB_CTRL_SUSPEND, OS_DRV_CPR_CLK_PLL0, 1, 10},
   {OS_DRV_CPR_DEV_CSS_AUX_USB20_REF, OS_DRV_CPR_CLK_PLL0, 1, 50},
   {OS_DRV_CPR_DEV_CSS_AUX_USB_CTRL_REF, OS_DRV_CPR_CLK_PLL0, 1, 600},
   {OS_DRV_CPR_DEV_CSS_AUX_USB_CTRL, OS_DRV_CPR_CLK_PLL0, 1, 2},
   OS_DRV_CPR_AUX_ARRAY_TERMINATOR
};

static OsDrvCprConfig config = {
    .ref0InClk = DEFAULT_OSC_CLOCK_KHZ,
    .auxCfg = auxClkConfig,
    .devCfg = devices
};
OS_DRV_INIT_CPR_CFG_DEFINE(&config);

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

