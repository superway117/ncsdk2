///
/// @file
/// @copyright All code copyright Movidius Ltd 2017, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     USB VSC main application
///

#include <stdlib.h>
#include <stdio.h>
#include <rtems.h>
#include <assert.h>
#include <rtems/bspIo.h>
#include <bsp.h>
#include <semaphore.h>

#include "OsDrvUsbPhy.h"
#include "OsDrvCpr.h"

#include "usbpumpdebug.h"
#include "usbpump_application_rtems_api.h"

#include "vsc2app_api.h"
#include "usbpump_vsc2app.h"

#include "app_config.h"
#include "rtems_config.h"

#include "OsDrvGpio.h"

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------
#ifndef DISABLE_LEON_DCACHE
# define USBPUMP_MDK_CACHE_ENABLE   1
#else
# define USBPUMP_MDK_CACHE_ENABLE   0
#endif

// Can be changed to 1 to add timeout functionality on the transfers
//#define TIMEOUT 1
//#define WAIT_TIME_MS 5000 // 5 sec timeout

#ifndef TIMEOUT
# define TIMEOUT 0
#endif

#ifndef WAIT_TIME_MS
# define WAIT_TIME_MS 2000
#endif

#define FIRST_ENDPOINT_INDEX 0
#define LAST_ENDPOINT_INDEX 2

#define PACKET_SIZE  512
#define MAX_TRANSFER 2*1024*1024 // 2MB

#define REPEAT_COUNT  500
#define TRANSFER_SIZE 1024*1024

// 3: Global Data

// USB VSC Handler
extern USBPUMP_VSC2APP_CONTEXT *  pSelf;

// 4: Static Local Data
// ----------------------------------------------------------------------------
static USBPUMP_APPLICATION_RTEMS_CONFIGURATION sg_DataPump_AppConfig =
USBPUMP_APPLICATION_RTEMS_CONFIGURATION_INIT_V3(
  /* nEventQueue */   64,
  /* pMemoryPool */   NULL,
  /* nMemoryPool */   0,
  /* DataPumpTaskPriority */  100,
  /* DebugTaskPriority */   200,
  /* UsbInterruptPriority */  10,
  /* pDeviceSerialNumber */ NULL,
  /* pUseBusPowerFn */    NULL,
  /* fCacheEnabled */   USBPUMP_MDK_CACHE_ENABLE,
  /* DebugMask */     UDMASK_ANY | UDMASK_ERRORS,
  /* pPlatformIoctlFn */    NULL,
  /* fDoNotWaitDebugFlush */  0
  );

static pthread_t rThread, wThread;

// Transfer buffers
static char rBuff[MAX_TRANSFER]__attribute__((aligned(64), section(".ddr_direct.bss")));
static char wBuff[MAX_TRANSFER]__attribute__((aligned(64), section(".ddr_direct.bss")));

sem_t  readwriteSyncSem; 



rtems_status_code read_device_id()
{
  void* gpio39_hdl;
  void* gpio40_hdl;
  void* gpio41_hdl;
  void* gpio42_hdl;
  void* gpio43_hdl;

  u8 pin_data1;
  u8 pin_data2;
  u8 pin_data3;
  u8 pin_data4;
  u8 pin_data5;

  rtems_status_code ret;

  ret = OsDrvGpioRequestExclusive(39,&gpio39_hdl);
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRequestExclusive(40,&gpio40_hdl);
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRequestExclusive(41,&gpio41_hdl);
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRequestExclusive(42,&gpio42_hdl);
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRequestExclusive(43,&gpio43_hdl);
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
 
  ret = OsDrvGpioSetMode(gpio39_hdl,OS_DRV_GPIO_MODE_7| OS_DRV_GPIO_DIR_IN);
  ret = OsDrvGpioSetMode(gpio40_hdl,OS_DRV_GPIO_MODE_7| OS_DRV_GPIO_DIR_IN);
  ret = OsDrvGpioSetMode(gpio41_hdl,OS_DRV_GPIO_MODE_7| OS_DRV_GPIO_DIR_IN);
  ret = OsDrvGpioSetMode(gpio42_hdl,OS_DRV_GPIO_MODE_7| OS_DRV_GPIO_DIR_IN);
  ret = OsDrvGpioSetMode(gpio43_hdl,OS_DRV_GPIO_MODE_7| OS_DRV_GPIO_DIR_IN);

 
  ret = OsDrvGpioRead(gpio39_hdl,&pin_data1);//device id0
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRead(gpio40_hdl,&pin_data2);//device id0
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRead(gpio41_hdl,&pin_data3);//device id0
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRead(gpio42_hdl,&pin_data4);//device id0
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  ret = OsDrvGpioRead(gpio43_hdl,&pin_data5);//device id0
  if(ret != RTEMS_SUCCESSFUL)
  {
    exit(ret);
  }
  OsDrvGpioRelease(gpio39_hdl);
  OsDrvGpioRelease(gpio40_hdl);
  OsDrvGpioRelease(gpio41_hdl);
  OsDrvGpioRelease(gpio42_hdl);
  OsDrvGpioRelease(gpio43_hdl);

  printf("\ndevice id:%d%d%d%d%d\n",pin_data1,pin_data2,pin_data3,pin_data4,pin_data5);

  wBuff[0]=(char)(pin_data1);
  wBuff[1]=(char)(pin_data2);
  wBuff[2]=(char)(pin_data3);
  wBuff[3]=(char)(pin_data4);
  wBuff[4]=(char)(pin_data5);
  return ret;

}


// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
 

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
static void fillWriteBuffer(char *buff, int size)
{
  int i, j;

  assert(PACKET_SIZE != 0);
  assert(size % PACKET_SIZE == 0);
  for(i = 0; i < size / PACKET_SIZE; i++)
    for(j = 0; j < PACKET_SIZE; j++)
      buff[i*PACKET_SIZE + j] = i + 1;
}

// Required for synchronisation between internal USB thread and our threads
static int32_t createSemaphores(void) {
  int32_t i, sc;

  for(i = 0; i < USBPUMP_VSC2APP_NUM_EP_IN; i++)
  {
    sc = sem_init(&pSelf->semWriteId[i], 0, 0);
    if (sc == -1)
      return sc;
  }

  for(i = 0; i < USBPUMP_VSC2APP_NUM_EP_OUT; i++)
  {
    sc = sem_init(&pSelf->semReadId[i], 0, 0);
    if (sc == -1)
      return sc;
  }
  sc = sem_init(&readwriteSyncSem, 0, 0);
  return 0;
}

static void * write_thread(void *args)
{
  UNUSED(args);
  int status;
  int endpoint_index;

  for (endpoint_index = FIRST_ENDPOINT_INDEX;
       endpoint_index <= LAST_ENDPOINT_INDEX; endpoint_index++)
  {
    // Wait for the interface up event
    status = sem_wait(&pSelf->semWriteId[endpoint_index]);
    for(int i = 0; i < REPEAT_COUNT; i++)
    {
      sem_wait(&readwriteSyncSem);
      printf("Starting write transfer [ep %d] %d bytes\n", endpoint_index + 1, TRANSFER_SIZE);
      UsbVscAppWrite(pSelf, TRANSFER_SIZE, wBuff, endpoint_index);

      if(TIMEOUT) {
        struct timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts);
        ts.tv_sec += WAIT_TIME_MS / 1000;
        ts.tv_nsec += WAIT_TIME_MS % 1000 * 1000;
        status = sem_timedwait(&pSelf->semWriteId[endpoint_index], &ts);
      } else {
        status = sem_wait(&pSelf->semWriteId[endpoint_index]);
      }
      if (status != 0) {
        perror("sem_timedwait:");
        break;
      }
    }
  }

  return NULL;
}

static void * read_thread(void *args)
{
  UNUSED(args);
  int status;
  int endpoint_index;

  for (endpoint_index = FIRST_ENDPOINT_INDEX;
       endpoint_index <= LAST_ENDPOINT_INDEX; endpoint_index++)
  {
    // Wait for the interface up event
    status = sem_wait(&pSelf->semReadId[endpoint_index]);
    for(int i = 0; i < REPEAT_COUNT; i++)
    {
      printf("Starting read transfer [ep %d] %d bytes\n", endpoint_index + 1, TRANSFER_SIZE);
      UsbVscAppRead(pSelf, TRANSFER_SIZE, rBuff, endpoint_index);
      fillWriteBuffer(rBuff, TRANSFER_SIZE);
      fillWriteBuffer(rBuff+TRANSFER_SIZE, TRANSFER_SIZE);
      read_device_id();
      sem_post(&readwriteSyncSem);
       
      if(TIMEOUT) {
        struct timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts);
        ts.tv_sec += WAIT_TIME_MS / 1000;
        ts.tv_nsec += WAIT_TIME_MS % 1000 * 1000;
        status = sem_timedwait(&pSelf->semReadId[endpoint_index], &ts);
      } else {
        status = sem_wait(&pSelf->semReadId[endpoint_index]);
      }

      if (status != 0) {
        perror("sem_timedwait:");
        break;
      }
    }
  }

  return NULL;
}


void *POSIX_Init (void *args)
{
  pthread_attr_t attr;
  int status;
  UNUSED(args);
  printf ("\nRTEMS POSIX Started\n");  /* initialise variables */

  status = OsDrvUsbPhyInit(NULL);
  if (status != RTEMS_SUCCESSFUL)
    exit(status);

  status = OsDrvGpioInit(NULL);
  if (status != RTEMS_SUCCESSFUL)
    exit(status);

  // generate write buffer
  fillWriteBuffer(wBuff, MAX_TRANSFER);

  //status =  read_device_id();
  //if (status != RTEMS_SUCCESSFUL)
  //  exit(status);


  if (!UsbPump_Rtems_DataPump_Startup(&sg_DataPump_AppConfig))
  {
    printf("\n\nRtems_DataPump_Startup() failed!\n\n\n");
    exit(1);
  }

  // create semaphores for transfer synchronization
  if(createSemaphores() == -1)
    printf("Error creating semaphores\n");

  // initialize pthread attr and create read and write threads
  if(pthread_attr_init(&attr) !=0)
    printf("pthread_attr_init error");
  if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0)
    printf("pthread_attr_setinheritsched error");
  if(pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0)
    printf("pthread_attr_setschedpolicy error");

  if (pthread_create( &wThread, &attr, &write_thread,0) != 0) {
    printf("Write thread creation failed!\n");
    exit(1);
  } else {
    printf("Write thread created\n");
  }
  if (pthread_create( &rThread, &attr, &read_thread,0) != 0) {
    printf("Read thread creation failed!\n");
    exit(1);
  } else {
    printf("Read thread created\n");
  }

  status = pthread_join( wThread, NULL);
  if(status != 0) {
    printf("wThread pthread_join error %d!\n", status);
    exit(1);
  } else {
    printf("Write thread complete\n");
  }

  status = pthread_join(rThread, NULL);
  if(status != 0) {
    printf("rThread pthread_join error %d!\n", status);
    exit(1);
  } else {
    printf("Read thread complete\n");
  }

  exit(0);
}

