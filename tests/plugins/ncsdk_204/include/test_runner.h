
#ifndef _TEST_RUNNER_INTERNAL_H_
#define _TEST_RUNNER_INTERNAL_H_

#include <pthread.h>
#include "mvnc.h"
#include "mvnc_ion.h"

#define MAX_GRAPH_NUM 10
#define MAX_TENSOR_NUM 10
#define MAX_DEVICE_NUM 32


typedef struct _TestRunnerProcess  TestRunnerProcess;


 
typedef struct _TESTDEVICE_HANDLER
{
  
  TestRunnerProcess* process;
  int index;
  int dev_idx;
  int writeThreadIsRun;

  char sw[127];
  struct ncDeviceHandle_t* device_handle;

  char graph_paths[MAX_GRAPH_NUM][127];
  struct ncGraphHandle_t *graph_handles[MAX_GRAPH_NUM];
  int graph_num;
 
  struct ncFifoHandle_t* fifo_in_handles[MAX_GRAPH_NUM];
  struct ncFifoHandle_t* fifo_out_handles[MAX_GRAPH_NUM];

  char tensor[127];

  char run_mode[10];
  int run_times;
  int inference_times;

  int ion;
  
  char to_format_str[50];
  int to_format;
  int class_num;

  
  
   
}TESTDEVICE_HANDLER;

typedef int (*TR_GET_RESULT_PRE)(TESTDEVICE_HANDLER* deviceParam);

typedef int (*TR_GET_RESULT_POST)(TESTDEVICE_HANDLER* deviceParam);


struct _TestRunnerProcess
{   
    TR_GET_RESULT_PRE           result_pre;
    TR_GET_RESULT_POST          result_post;
 
};


extern int test_runner_create(const char* cfg_path,TestRunnerProcess* process);

extern int test_runner_run();

extern pthread_t gThreadNvaList[MAX_DEVICE_NUM];

extern TESTDEVICE_HANDLER gDeviceParamList[MAX_DEVICE_NUM];


extern void mvnc_runinference(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_output_fifo_delete(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_input_fifo_delete(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_fifo_init_and_create(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_dealloc_graph(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_alloc_graph(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_graph_init(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_device_close(TESTDEVICE_HANDLER* deviceParam);

extern int mvnc_device_open(TESTDEVICE_HANDLER* deviceParam);

extern void mvnc_device_init();
#endif




