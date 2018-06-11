/*
* Copyright 2018 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
*/

///
/// @file
///
/// @brief     MVNC host-device communication structures
///


// Includes
// ----------------------------------------------------------------------------

#ifndef _NC_HIGH_CLASS_H_
#define _NC_HIGH_CLASS_H_
#ifdef __cplusplus
extern "C" {
#endif

#if (defined (WINNT) || defined(_WIN32) || defined(_WIN64) )

#include "mvnc.h"
#define dllexport __declspec( dllexport )
#else
#define dllexport
#endif

#define NC_MAX_OPTIMISATIONS       40
#define NC_OPTIMISATION_NAME_LEN   50
#define NC_OPTIMISATION_LIST_BUFFER_SIZE (NC_MAX_OPTIMISATIONS * NC_OPTIMISATION_NAME_LEN)

typedef enum {
    NC_RW_GRAPH_BATCH_SIZE = 1201,  // configure batch size. 1 by default
    NC_RW_GRAPH_RUNTIME_CONFIG = 1202,  //blob config file with resource hints

    GRAPH_2_NOT_IMPLEMENTED = 1203,
} ncGraphOptionClass2_t;

typedef enum {
    GRAPH_3_NOT_IMPLEMENTED = 1300,
} ncGraphOptionClass3_t;


typedef enum {
    NC_RW_DEVICE_TEMP_LIM_LOWER = 2200, // Temperature for short sleep, float, not for general use
    NC_RW_DEVICE_TEMP_LIM_HIGHER = 2201,    // Temperature for long sleep, float, not for general use
    NC_RW_DEVICE_BACKOFF_TIME_NORMAL = 2202,    // Normal sleep in ms, int, not for general use
    NC_RW_DEVICE_BACKOFF_TIME_HIGH = 2203,  // Short sleep in ms, int, not for general use
    NC_RW_DEVICE_BACKOFF_TIME_CRITICAL = 2204,  // Long sleep in ms, int, not for general use
    NC_RW_DEVICE_TEMPERATURE_DEBUG = 2205,  // Stop on critical temperature, int, not for general use
    NC_RO_DEVICE_OPTIMISATION_LIST = 2206,      // Return optimisations list, char *, not for general use

} ncDeviceOptionClass2_t;


typedef enum {
    NC_RW_DEVICE_SHELL_ENABLE = 2300,   // Activate RTEMS shell.
    NC_RW_DEVICE_LOG_LEVEL = 2301,  // Set/Get log level for the NC infrastructure on the device
    NC_RW_DEVICE_MVTENSOR_LOG_LEVEL = 2302, //Set/Get log level of MV_Tensor
    NC_RW_DEVICE_XLINK_LOG_LEVEL = 2303,    //Set.Get XLink log level
    DEV_3_NOT_IMPLEMENTED = 2304
} ncDeviceOptionClass3_t;


dllexport ncStatus_t setDeviceOptionClass2(struct _devicePrivate_t *d,
                                           int option,
                                           const void *data,
                                           unsigned int dataLength);
dllexport ncStatus_t setDeviceOptionClass3(struct _devicePrivate_t *d,
                                           int option, const void *data,
                                           unsigned int dataLength);


dllexport ncStatus_t getDeviceOptionClass2(struct _devicePrivate_t *d,
                                           int option,
                                           void *data,
                                           unsigned int *dataLength);
dllexport ncStatus_t getDeviceOptionClass3(struct _devicePrivate_t *d,
                                           int option, void *data,
                                           unsigned int *dataLength);

dllexport ncStatus_t getGraphOptionClass2(struct _graphPrivate_t *g,
                                          int option,
                                          void *data,
                                          unsigned int *dataLength);
dllexport ncStatus_t getGraphOptionClass3(struct _graphPrivate_t *g,
                                          int option, void *data,
                                          unsigned int *dataLength);
dllexport ncStatus_t setGraphOptionClass2(struct _graphPrivate_t *g,
                                          int option, const void *data,
                                          unsigned int dataLength);
dllexport ncStatus_t setGraphOptionClass3(struct _graphPrivate_t *g,
                                          int option, const void *data,
                                          unsigned int dataLength);

#ifdef __cplusplus
}
#endif
#endif
