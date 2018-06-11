
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
//#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include <json-c/json.h>

#include <sys/mman.h>
#include <assert.h>
#ifdef TEST_USE_ION
#include <ion_kernel.h>
#include <ion.h>
#include <mvnc_ion.h>
#include <ncHighClass.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"


#include "test_runner.h"

#include "utils.h"
#ifndef  ACCEPT_TEST
#include "utils_ion.h"
#endif

typedef struct 
{
  TESTDEVICE_HANDLER* deviceParam;
  int graph_index;
  int is_read_thread;
}ReadParamStruct;


pthread_t gThreadNvaList[MAX_DEVICE_NUM];

TESTDEVICE_HANDLER gDeviceParamList[MAX_DEVICE_NUM];
ReadParamStruct s_read_param[MAX_DEVICE_NUM][MAX_GRAPH_NUM];
int ion_fd;

void mvnc_device_init() {
  int i;
  
  for (i = 0; i < MAX_DEVICE_NUM; i++) {
    if (gDeviceParamList[i].dev_idx == -1) {
      continue;
    }

    ncStatus_t rc;
    struct ncDeviceHandle_t* dH;
    rc = ncDeviceCreate(gDeviceParamList[i].dev_idx, &dH);
    if (rc) {
      printf("No device found for idx[%d].\n", gDeviceParamList[i].dev_idx);
      gDeviceParamList[i].dev_idx = -1;
    } else {
      gDeviceParamList[i].device_handle = (void*)dH;
    }
  }
}

int mvnc_device_open(TESTDEVICE_HANDLER* deviceParam) {
  if (deviceParam->dev_idx == -1) {
    return -1;
  }

  ncStatus_t rc;
  struct ncDeviceHandle_t* dH =
      (struct ncDeviceHandle_t*)deviceParam->device_handle;

  rc = ncDeviceOpen(dH);
  if (rc) {
    printf("[idx %d]ncDeviceOpen failed, rc=%d\n", deviceParam->dev_idx, rc);
    deviceParam->dev_idx = -1;
    return -1;
  }
  printf("\033[41;36m [idx %d]Open Device succeeded-------------\033[0m\n",
         deviceParam->dev_idx);
  /*
  int portnum = 40000;
int level2 = 5;
rc = ncDeviceSetOption(dH,  NC_RW_DEVICE_SHELL_ENABLE, &portnum, sizeof(portnum));
rc += ncDeviceSetOption(dH,  NC_RW_DEVICE_LOG_LEVEL, &level2, sizeof(level2));
rc += ncDeviceSetOption(dH,  NC_RW_DEVICE_MVTENSOR_LOG_LEVEL, &level2, sizeof(level2));
rc += ncDeviceSetOption(dH,  NC_RW_DEVICE_XLINK_LOG_LEVEL, &level2, sizeof(level2));
  */
  return 0;
}

int mvnc_device_close(TESTDEVICE_HANDLER* deviceParam) {
  ncStatus_t rc;
  struct ncDeviceHandle_t* dH =
      (struct ncDeviceHandle_t*)deviceParam->device_handle;
  rc = ncDeviceClose(dH);
  if (rc) {
    printf("[idx %d]ncDeviceClose failed, rc=%d\n", deviceParam->dev_idx, rc);
    return -1;
  }
  printf("[idx %d]mvnc_device_close rc = %d\n", deviceParam->dev_idx, rc);
  return 0;
}

int mvnc_graph_init(TESTDEVICE_HANDLER* deviceParam) {
  ncStatus_t rc;
  int i=0;
  char graph_name[20];
  for(;i<deviceParam->graph_num;i++)
  {
    struct ncGraphHandle_t* gH;
    sprintf(graph_name,"TestGraph%d",i);
    rc = ncGraphCreate(graph_name, &gH);
    if (rc) {
      printf("\033[41;36m [idx %d]ncGraphInit failed rc:%d\033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
    deviceParam->graph_handles[i] = (void*)gH;
  }
  return 0;
}

int mvnc_alloc_graph(TESTDEVICE_HANDLER* deviceParam) {
  ncStatus_t rc;
  unsigned int length;
   
  unsigned int mem, memMax;
  int i=0;
 
  struct ncDeviceHandle_t* deviceHandle =
      (struct ncDeviceHandle_t*)deviceParam->device_handle;


  int ret = -1;
  char* graphFile;
  unsigned int graphSize;
  for(i=0;i<deviceParam->graph_num;i++)
  {


    ret = load_graph_file(deviceParam->graph_paths[i], &graphFile, &graphSize);

    if (ret) {
      printf("\033[41;36m [index %d]mvnc_alloc_graph failed to load %s\033[0m\n",
             deviceParam->dev_idx, deviceParam->graph_paths[i]);
      return -1;
    }

    // check device mem before alloc
    
    struct ncGraphHandle_t* graphHandle =
        (struct ncGraphHandle_t*)deviceParam->graph_handles[i];
    length = sizeof(unsigned int);
    rc = ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_CURRENT_MEMORY_USED, (void **)&mem, &length);
    rc += ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_MEMORY_SIZE, (void **)&memMax, &length);
    if(rc)
      printf("ncDeviceGetOption failed, rc=%d\n", rc);
    else
      printf("Current memory used on device is %d out of %d\n", mem, memMax);


    rc = ncGraphAllocate(deviceHandle, graphHandle, graphFile, graphSize);
      if (rc != NC_OK)
      {   // error allocating graph
          printf("Could not allocate graph for file: %s\n", deviceParam->graph_paths[i]); 
          printf("Error from ncGraphAllocate is: %d\n", rc);
          return false;
      }
   

    free(graphFile);
  }

  return 0;
}
 

int mvnc_dealloc_graph(TESTDEVICE_HANDLER* deviceParam) {
  ncStatus_t rc;
  int i=0;
  for(;i<deviceParam->graph_num;i++)
  {
    struct ncGraphHandle_t* gH = (struct ncGraphHandle_t*)deviceParam->graph_handles[i];
    rc = ncGraphDestroy(&gH);
    deviceParam->graph_handles[i] = NULL;
    if (rc) {
      printf("[index %d]ncGraphClose failed, rc=%d\n", deviceParam->dev_idx, rc);
      return -1;
    }
    printf("[index %d]mvnc_dealloc_graph rc = %d\n", deviceParam->dev_idx, rc);
  }
  return 0;
}

int mvnc_fifo_init_and_create(TESTDEVICE_HANDLER* deviceParam) {
  int rc = -1;


  struct ncDeviceHandle_t* dH =
      (struct ncDeviceHandle_t*)deviceParam->device_handle;
  int i=0;
  for(;i<deviceParam->graph_num;i++)
  {
    struct ncGraphHandle_t* gH = (struct ncGraphHandle_t*)deviceParam->graph_handles[i];

    unsigned int length;
    struct ncTensorDescriptor_t inputTensorDesc;
    struct ncTensorDescriptor_t outputTensorDesc;
    length = sizeof(struct ncTensorDescriptor_t);
    ncGraphGetOption(gH, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTensorDesc,  &length);
    ncGraphGetOption(gH, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTensorDesc,  &length);


   
    // Initialize & Create Fifos
    int numElem = 2;
    struct ncFifoHandle_t * inputFifoHandle;
    struct ncFifoHandle_t * outputFifoHandle;
    char infifo_name[20];
    sprintf(infifo_name,"fifoIn%d-%d",deviceParam->dev_idx,i);
    char outfifo_name[20];
    sprintf(outfifo_name,"fifoOut%d-%d",deviceParam->dev_idx,i);
    rc = ncFifoCreate(infifo_name, NC_FIFO_HOST_WO, &inputFifoHandle);
    rc += ncFifoAllocate(inputFifoHandle, dH, &inputTensorDesc, numElem);
    rc += ncFifoCreate(outfifo_name, NC_FIFO_HOST_RO, &outputFifoHandle);
    rc += ncFifoAllocate(outputFifoHandle, dH, &outputTensorDesc, numElem);

    if (rc) {
      printf("\033[41;36m [idx %d]mvnc_fifo_init_and_create faided rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
    deviceParam->fifo_in_handles[i] = (void*)inputFifoHandle;
    deviceParam->fifo_out_handles[i] = (void*)outputFifoHandle;
    
    printf("[devIdx:%d;graphIdx:%d]mvnc_fifo_init_and_create ok\n",
             deviceParam->dev_idx, i);
  }

  return 0;
}

int mvnc_input_fifo_delete(TESTDEVICE_HANDLER* deviceParam) {
  int rc = -1;
  int i=0;
   
  for(;i<deviceParam->graph_num;i++)
  {
    struct ncFifoHandle_t* fH = (struct ncFifoHandle_t*)deviceParam->fifo_in_handles[i];
     
    #ifdef TEST_USE_ION
    rc = ncFifoDelete(fH);
    if (rc) {
      printf("\033[41;36m [idx %d]mvnc_input_fifo_delete  ncFifoDelete faided rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
    #endif

    rc = ncFifoDestroy(&fH);
    if (rc) {
      printf("\033[41;36m [idx %d]mvnc_input_fifo_delete ncFifoDestroy faided rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
  }
  return 0;
}


int mvnc_output_fifo_delete(TESTDEVICE_HANDLER* deviceParam) {
  int rc = -1;
  int i=0;
 
  for(;i<deviceParam->graph_num;i++)
  {
 
    struct ncFifoHandle_t* fH = (struct ncFifoHandle_t*)deviceParam->fifo_out_handles[i];
    #ifdef TEST_USE_ION
    rc = ncFifoDelete(fH);
    if (rc) {
      printf("\033[41;36m [idx %d]mvnc_output_fifo_delete ncFifoDelete faided rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
    #endif

    rc = ncFifoDestroy(&fH);
    if (rc) {
      printf("\033[41;36m [idx %d]mvnc_output_fifo_delete ncFifoDestroy faided rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
      return -1;
    }
  }
  return 0;
}
 

void* readResultThread(void* thread_param) {
  ReadParamStruct* read_param = (ReadParamStruct*)thread_param;
  TESTDEVICE_HANDLER* deviceParam = (TESTDEVICE_HANDLER*)read_param->deviceParam;
  int graph_index = read_param->graph_index;
  int is_read_thread = read_param->is_read_thread;
  printf("[%s:%d][devIdx:%d;graphIdx:%d]readResultThread start.....\n",__func__,__LINE__,deviceParam->dev_idx,graph_index);
  ncStatus_t rc = 0;
   
  struct ncFifoHandle_t* outputFifoHandle =
      (struct ncFifoHandle_t*)deviceParam->fifo_out_handles[graph_index];

  struct ncGraphHandle_t* graphHandle =
      (struct ncGraphHandle_t*)deviceParam->graph_handles[graph_index];
  
  /*
  unsigned int length;
  unsigned int outputDataLength;
    length = sizeof(unsigned int);
    rc = ncFifoGetOption(outputFifoHandle, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (rc || length != sizeof(unsigned int)){
        printf("ncFifoGetOption failed, rc=%d\n", rc);
        exit(-1);
    }
  */
  #ifdef TEST_USE_ION
  unsigned int length = sizeof(struct ncTensorDescriptor_t);
  struct ncTensorDescriptor_t outputTensorDesc;
  ncGraphGetOption(graphHandle, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS,
                   (void*)&outputTensorDesc, &length);

 unsigned int outputDataLength = outputTensorDesc.totalSize;
 #else
 unsigned int length;
  unsigned int outputDataLength;
    length = sizeof(unsigned int);
    rc = ncFifoGetOption(outputFifoHandle, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (rc || length != sizeof(unsigned int)){
        printf("ncFifoGetOption failed, rc=%d\n", rc);
        exit(-1);
    }
 #endif
 
  float* resultData = (float*) malloc(outputDataLength);
  void* userParam;

  #ifdef TEST_USE_ION
  int outputShareFd = -1;
  ion_user_handle_t handle;
  ion_phys_addr_t physAddr, cpuAddr;
  if(deviceParam->ion == 1)
  {
    rc = ion_alloc(ion_fd, outputDataLength, align, heap_mask, alloc_flags, &handle,
                   &physAddr, &cpuAddr);
    rc += ion_share(ion_fd, handle, &outputShareFd);
    rc += ion_free(ion_fd, handle);
    if (rc) {
      printf("readResult ion alloc failed\n");
    }
  }

  #endif
  int frames = 0;
  do
  {
   
    //printf("deviceParam->process->result_pre = 0x%p\n",deviceParam->process->result_pre);
    if(deviceParam->process && deviceParam->process->result_pre)
    {
      deviceParam->process->result_pre(deviceParam);
    }
 
    #ifdef TEST_USE_ION
    if(deviceParam->ion == 1)
      rc = ncFifoReadIonElem(outputFifoHandle, outputShareFd, &outputDataLength,
                           &userParam);
    else
    #endif
      rc = ncFifoReadElem(outputFifoHandle, (void*) resultData, &outputDataLength, &userParam);
    if (rc != NC_OK)
    {
      printf("\033[41;36m [idx %d]GetResult failed, rc=%d \033[0m\n",
             deviceParam->dev_idx, rc);
       
      assert(0);
    }
 
    #ifdef TEST_USE_ION
    if(deviceParam->ion == 1){
      
      void* result;
      result = mmap(NULL, outputDataLength, prot, map_flags, outputShareFd, 0);
      unsigned char* finalResult = (unsigned char*)malloc(outputDataLength);
      memcpy(finalResult, result, outputDataLength);
      memset(result, 0, outputDataLength);
      munmap(result, outputDataLength);
      printf("[idx %d]finalResult [1--3] 0x%x 0x%x 0x%x\n", deviceParam->dev_idx,
             finalResult[0], finalResult[1], finalResult[2]);
      free(finalResult);
      
 
    }
    else

    #endif
      printf("[idx %d]finalResult [1--3] 0x%x 0x%x 0x%x\n", deviceParam->dev_idx,
             resultData[0], resultData[1], resultData[2]);

    
    unsigned int numResults = outputDataLength/sizeof(float);
    float maxResult = 0.0;
    int maxIndex = -1;
    for (int index = 0; index < numResults; index++)
    {
       
        if (resultData[index] > maxResult)
        {
            maxResult = resultData[index];
            maxIndex = index;
        }
    }
 
    //printf("deviceParam->process->result_pre = 0x%p\n",deviceParam->process->result_pre);
    printf("\033[1;34m[idx %d] readThread got frame %d/%d return numResults=%d\n\033[0m", deviceParam->dev_idx,++frames,deviceParam->inference_times,numResults);
    printf("Index of top result is: %d\n", maxIndex);
    printf("Probability of top result is: %f\n", resultData[maxIndex]);
    //assert(resultData[maxIndex]>0.9);
    
    int timeTakenArraySize;
    unsigned int dataLength = sizeof(int);
    ncGraphGetOption(graphHandle, NC_RO_GRAPH_TIME_TAKEN_ARRAY_SIZE, &timeTakenArraySize, &dataLength);


    float *timeTaken = (float*)malloc(timeTakenArraySize);
    unsigned int timeTakenLen = timeTakenArraySize;
    printf("%p\n",timeTaken);
    rc = ncGraphGetOption(graphHandle, NC_RO_GRAPH_TIME_TAKEN,
                          (void*)timeTaken, &timeTakenLen);
    printf("%p\n",timeTaken);
    if (rc) {
      printf("[idx %d]ncGraphGetOption timeTaken failed, rc=%d\n",
             deviceParam->dev_idx, rc);
    } else {
      printf("\033[1;34m[idx %d]timeTaken = %f\033[0m\n", deviceParam->dev_idx, timeTaken[timeTakenLen/sizeof(float)-1]);

    }
    
    int throttling;
    unsigned int throttlingLength = sizeof(int);
    rc = ncDeviceGetOption(deviceParam->device_handle, NC_RO_DEVICE_THERMAL_THROTTLING_LEVEL,
                           (void*)&throttling, &throttlingLength);
    
    if(rc)
    {
      printf("[idx %d]ncDeviceGetOption failed, rc=%d\n", deviceParam->dev_idx,
             rc);
    }
    else
    {
      if (throttling == 1)
        printf("** NCS temperature high - thermal throttling initiated **\n");
      else if(throttling == 2)
      {
        printf("*********************** WARNING *************************\n");
        printf("* NCS temperature critical                              *\n");
        printf("* Aggressive thermal throttling initiated               *\n");
        printf("* Continued use may result in device damage             *\n");
        printf("*********************************************************\n");
        assert(0);
      
      }


    } 
 

    if(deviceParam->process && deviceParam->process->result_post)
    {
      deviceParam->process->result_post(deviceParam);
    }
 
    
  }while(frames<deviceParam->inference_times && is_read_thread==1);
  printf("readResultThread exit\n");
  #ifdef TEST_USE_ION
  if(deviceParam->ion == 1){
      
    close(outputShareFd);
  }
  #endif
  free(resultData);
  return NULL;
}

 
float *LoadImage(const char *path, int reqSize, float *mean)
{
    int width, height, cp, i;
    unsigned char *img, *imgresized;
    float *imgfp32;

    img = stbi_load(path, &width, &height, &cp, 3);
    if(!img)
    {
        printf("Error - the image file %s could not be loaded\n", path);
        return NULL;
    }
    imgresized = (unsigned char*) malloc(3*reqSize*reqSize);
    if(!imgresized)
    {
        free(img);
        perror("malloc");
        return NULL;
    }
    stbir_resize_uint8(img, width, height, 0, imgresized, reqSize, reqSize, 0, 3);
    free(img);
    imgfp32 = (float*) malloc(sizeof(*imgfp32) * reqSize * reqSize * 3);
    if(!imgfp32)
    {
        free(imgresized);
        perror("malloc");
        return 0;
    }
    for(i = 0; i < reqSize * reqSize * 3; i++)
        imgfp32[i] = imgresized[i];
    free(imgresized);

    for(i = 0; i < reqSize*reqSize; i++)
    {
        float blue, green, red;
        blue = imgfp32[3*i+2];
        green = imgfp32[3*i+1];
        red = imgfp32[3*i+0];

        imgfp32[3*i+0] = blue-mean[0];
        imgfp32[3*i+1] = green-mean[1];
        imgfp32[3*i+2] = red-mean[2];
    }
    return imgfp32;
}
void mvnc_runinference(TESTDEVICE_HANDLER* deviceParam) {
  ncStatus_t rc;


  int networkDimGoogleNet = 224;
  //int networkDimSqueezeNet = 227;
  float networkMeanGoogleNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};
  //float networkMeanSqueezeNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

  int networkDim = networkDimGoogleNet;
  float *mean = networkMeanGoogleNet;

  
  // LoadImage will read image from disk, convert channels to floats
  // subtract network mean for each value in each channel.  Then, 
  // return pointer to the buffer of 32Bit floats
  printf("tensor path is %s\n",deviceParam->tensor);
  char* imageBuf = NULL;
  int path_len = strlen(deviceParam->tensor);
  int is_raw = 1;
  if(path_len>3 && deviceParam->tensor[path_len-4]=='.')
  {
    is_raw = 0;
  }
  unsigned int binLength = 0;
  #ifdef TEST_USE_ION
  int shareFd;
 
  if(deviceParam->ion==1)
    load_test_bin_ion(deviceParam->tensor, &binLength, &shareFd);
  else
  #endif
  {
    if(is_raw == 0)
    {
      imageBuf= (char*)LoadImage(deviceParam->tensor, networkDim, mean);
      binLength = 3*networkDim*networkDim*sizeof(float);
    }
    else
    {
      
      load_test_bin(deviceParam->tensor, &binLength, &imageBuf);
    }
  }
  printf("binLength %d\n",binLength);
 
  //unsigned int lenBuf = 3*networkDim*networkDim*sizeof(*imageBuf);
  int i = 0;
  int j = 0;
  int sync_mode = 0;
  if (0!=strcmp("async",deviceParam->run_mode))
    sync_mode = 1;
  pthread_t readThread[MAX_GRAPH_NUM];
  

  deviceParam->writeThreadIsRun =1;

  if(sync_mode==0)
  {
    for(i=0;i<deviceParam->graph_num;i++)
    {
      s_read_param[deviceParam->dev_idx][i].deviceParam = deviceParam;
      s_read_param[deviceParam->dev_idx][i].graph_index = i;
      s_read_param[deviceParam->dev_idx][i].is_read_thread = 1;
      pthread_create(&readThread[i], 0, readResultThread, &s_read_param[deviceParam->dev_idx][i]);
    }
    
  }
  

  printf("binLength %d\n",binLength);
  for (i=0; i < deviceParam->inference_times; i++)
  {
    printf("\033[1;32m[idx %d]Sending the %dth frame to device\033[0m\n", deviceParam->dev_idx, i);
    for(j=0;j<deviceParam->graph_num;j++)
    {
      struct ncFifoHandle_t* inputFifoHandle =
      (struct ncFifoHandle_t*)deviceParam->fifo_in_handles[j];
      struct ncFifoHandle_t* outputFifoHandle =
      (struct ncFifoHandle_t*)deviceParam->fifo_out_handles[j];
      struct ncGraphHandle_t* graphHandle =
      (struct ncGraphHandle_t*)deviceParam->graph_handles[j];
      printf("deviceParam->ion == %d 0000\n",deviceParam->ion);
      #ifdef TEST_USE_ION
      printf("deviceParam->ion == %d shareFd=%d\n",deviceParam->ion,shareFd);
      if(deviceParam->ion==1)
      {
    
        rc = ncFifoWriteIonElem(inputFifoHandle, (void*)((int64_t)shareFd), NULL, NULL);
      }
      else
      #endif
        rc = ncFifoWriteElem(inputFifoHandle, imageBuf, &binLength, NULL);
      printf("[%s:%d][devIdx:%d;graphIdx:%d]start load tensor .....\n",__func__,__LINE__,deviceParam->dev_idx,j);

      if (rc != NC_OK)
      {   // error loading tensor
          printf("Error - Could not load tensor rc=%d\n",rc);
          printf("    ncStatus_t from mvncLoadTensor is: %d\n", rc);
          free(imageBuf);
          assert(0);
          return;
      }


      rc = ncGraphQueueInference(graphHandle, &inputFifoHandle, 1, &outputFifoHandle, 1);
      if (rc)
      {
          free(imageBuf);
          printf("ncGraphQueueInference failed, rc=%d\n", rc);
          assert(0);
          return;
      }
      if(sync_mode==1)
      {
        s_read_param[deviceParam->dev_idx][j].deviceParam = deviceParam;
        s_read_param[deviceParam->dev_idx][j].graph_index =j;
        s_read_param[deviceParam->dev_idx][j].is_read_thread = 0;
        readResultThread(&s_read_param[deviceParam->dev_idx][j]);
      }
    }
  }
  deviceParam->writeThreadIsRun =0;
  printf("mvnc_runinference  is fininshed 0\n");
  free(imageBuf);
  void* retval[MAX_GRAPH_NUM];
  if(sync_mode==0)
  {
    for(i=0;i<deviceParam->graph_num;i++)
    {
      
      pthread_join(readThread[i], &retval[i]);
    }
    
  }
  printf("mvnc_runinference  is fininshed\n");
}

void sigint_handler(int signo) {
  printf("sigint_handler catch signal %d\n", signo);
  if (signo == SIGINT) {
    printf("SIGINT stop!!!\n");

    exit(0);
  }
  return;
}

void* runner_thread_main(void* gDeviceParamList_i) {
  int ret = -1;

  TESTDEVICE_HANDLER* deviceParam = (TESTDEVICE_HANDLER*)gDeviceParamList_i;

  printf("\033[41;36m runner_thread_main for %d \033[0m\n", deviceParam->dev_idx);

  ret = mvnc_device_open(deviceParam);
  if (ret) {
    printf("device open failed\n");
    return 0;
  }

  int testTime = 0;

  for(testTime=0;testTime<deviceParam->run_times;testTime++)
  {

    printf("\033[1;32m[idx %d]RunTime %dth\033[0m\n", deviceParam->dev_idx, testTime);
    printf("\033[1;32m----------------------------------------\033[0m\n");
    ret = mvnc_graph_init(deviceParam);
    if (ret) {
      printf("graph init failed\n");
      return 0;
    }

    ret = mvnc_alloc_graph(deviceParam);
    if (ret) {
      printf("test[%d]Alloc Graph failed\n", testTime);
      return 0;
    }

    ret = mvnc_fifo_init_and_create(deviceParam);
    if (ret) {
      printf("test[%d]create fifo_in failed\n", testTime);
      return 0;
    }

    if(deviceParam->inference_times>0)
      mvnc_runinference(deviceParam);

    ret = mvnc_input_fifo_delete(deviceParam);
    if (ret) {
      printf("test[%d]Delete fifo_in failed\n", testTime);
      return 0;
    }

    ret = mvnc_output_fifo_delete(deviceParam);
    if (ret) {
      printf("test[%d]Delete fifo_out failed\n", testTime);
      return 0;
    }
    mvnc_dealloc_graph(deviceParam);

  }
  

  mvnc_device_close(deviceParam);

  printf("\033[41;36m runner_thread_main %d end...... \033[0m\n",
         deviceParam->dev_idx);
  gDeviceParamList[deviceParam->index].dev_idx = -1;
  gDeviceParamList[deviceParam->index].device_handle = NULL;

  
  return 0;
}

 
int test_runner_create(const char* cfg_path,TestRunnerProcess* process)
{
  int i = 0;
  int logLevel = 1;
  assert(cfg_path);
  //assert(process);

  if(process)
  {
    if(process->result_pre)
    {
      printf("result_pre = %p\n",process->result_pre);
    }
  }

  for (i = 0; i < MAX_DEVICE_NUM; i++) {
    memset(&gDeviceParamList[i], 0, sizeof(gDeviceParamList[i]));
    gDeviceParamList[i].dev_idx = -1;
    gDeviceParamList[i].process = process;
  }

  load_test_runner_cfg(cfg_path, gDeviceParamList);
  //for (i = 0; i < MAX_DEVICE_NUM; i++) {
  //  gDeviceParamList[i].process = process;
  //}
  ncGlobalSetOption(NC_RW_LOG_LEVEL, &logLevel, sizeof(logLevel));
  mvnc_device_init();
  return 0;

}

int test_runner_run()
{
  int i;
  #ifdef TEST_USE_ION
  ion_fd = ion_open();
  #endif
  for (i = 0; i < MAX_DEVICE_NUM; i++)
  {
    if (gDeviceParamList[i].dev_idx == -1)
      break;
    pthread_create(&gThreadNvaList[i], 0, runner_thread_main,
                   &gDeviceParamList[i]);
  }
  void* retval;
  for (i = 0; i < MAX_DEVICE_NUM; i++)
  {
    if (gDeviceParamList[i].dev_idx == -1)
      break;
    pthread_join(gThreadNvaList[i], &retval);
  }
  #ifdef TEST_USE_ION
  ion_close(ion_fd);
  #endif
  printf("Main Func Exit Successfully!\n");
  return 0;

}
