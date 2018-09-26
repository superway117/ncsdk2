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
/// @brief     Application configuration Leon header
///

#ifndef _XLINK_USBLINKPLATFORM_H
#define _XLINK_USBLINKPLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_POOLS_ALLOC 32
#define PACKET_LENGTH (64*1024)
//PCIe 
#define PCIE_MAX_BUFFER_SIZE (255 * 4096)
#ifdef __PC__
#define MAX_LINKS 16
#else
#define MAX_LINKS 1
#endif

/*
heep this struc tduplicate for now. think of a different solution*/
typedef enum{
    UsbVSC = 0,
    UsbCDC,
    Pcie,
    Ipc,
    protocols
} protocol_t;

int XLinkWrite(void* fd, void* data, int size, unsigned int timeout);
int XLinkRead(void* fd, void* data, int size, unsigned int timeout);
int XLinkPlatformConnect(const char* devPathRead,
                           const char* devPathWrite, void** fd);
int XLinkPlatformInit(protocol_t protocol, int loglevel);

int XLinkPlatformGetDeviceName(int index,
                                char* name,
                                int nameSize);
int XLinkPlatformGetDeviceNameExtended(int index,
                                char* name,
                                int nameSize,
                                int pid);

int XLinkPlatformBootRemote(const char* deviceName,
							const char* binaryPath);
int XLinkPlatformResetRemote(void* fd);

void* allocateData(uint32_t size, uint32_t alignment);
void deallocateData(void* ptr,uint32_t size, uint32_t alignment);

typedef enum usbLinkPlatformErrorCode {
    X_LINK_PLATFORM_SUCCESS = 0,
    X_LINK_PLATFORM_ERROR,
    X_LINK_PLATFORM_DEVICE_NOT_FOUND,
    X_LINK_PLATFORM_TIMEOUT
} usbLinkPlatformErrorCode_t;

#ifdef __cplusplus
}
#endif

#endif

/* end of include file */
