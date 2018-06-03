#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#include <json-c/json.h>
 
#include "test_runner.h"

 
 
void getTime(struct timespec nowTime) {
  char strTime[128];
  clock_gettime(CLOCK_REALTIME, &nowTime);
  struct tm info;
  localtime_r(&nowTime.tv_sec, &info);  // thread safe.
  snprintf(strTime, 128, "[%02d:%02d:%02d.%09ld]\n", info.tm_hour, info.tm_min,
           info.tm_sec, nowTime.tv_nsec);
  printf("%s", strTime);
}


//===================== Parse Config ======================//
int load_test_runner_cfg(const char* file_path, void* param) {
  FILE* fp;
  char* buf;
  int i = 0;

  TESTDEVICE_HANDLER* s_param_list = (TESTDEVICE_HANDLER*)param;

  fp = fopen(file_path, "r");
  if (fp == NULL)
  {
    printf("config file is not exist\n");
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  unsigned int length = ftell(fp);
  rewind(fp);

  if (!(buf = (char*)malloc(length))) {
    printf("malloc failed\n");
    fclose(fp);
    return -1;
  }
  if (fread(buf, 1, length, fp) != length) {
    printf("read failed\n");
    fclose(fp);
    free(buf);
    return -1;
  }

  fclose(fp);

  struct json_object* obj = json_tokener_parse(buf);
  if (obj == NULL) {
    printf("json object is null\n");
    free(buf);
    return -1;
  }
  length = json_object_array_length(obj);
  printf("%d\n", length);
  for (i = 0; i < length; i++) {
    printf("\033[41;36m-----load cfg index=%d----------------\033[0m\n", i);
    s_param_list[i].index = i;
    struct json_object* jobj = json_object_array_get_idx(obj, i);

 
    struct json_object* jvalue = json_object_object_get(jobj, "tensor");
    strcpy((char*)&s_param_list[i].tensor, json_object_get_string(jvalue));

    printf("tensor:%s\n", s_param_list[i].tensor);

    jvalue = json_object_object_get(jobj, "graph_paths");
    int graph_length = json_object_array_length(jvalue);
    int j = 0;
    printf("graph_path length:%d\n",graph_length);
    for (j = 0; j < graph_length; j++)
    {
      printf("graph_path index:%d\n",j);
      struct json_object* graph_obj = json_object_array_get_idx(jvalue, j);

      strcpy((char*)&s_param_list[i].graph_paths[j], json_object_get_string(graph_obj));
      printf("graph_path:%s\n", s_param_list[i].graph_paths[j]);
    }
    //strcpy((char*)&s_param_list[i].graph_path, json_object_get_string(jvalue));
    s_param_list[i].graph_num = j;
    printf("graph_num:%d\n", s_param_list[i].graph_num);
 
    jvalue = json_object_object_get(jobj, "sw");
    strcpy((char*)&s_param_list[i].sw, json_object_get_string(jvalue));

    printf("sw:%s\n", s_param_list[i].sw);

    jvalue = json_object_object_get(jobj, "run_mode");
    strcpy((char*)&s_param_list[i].run_mode, json_object_get_string(jvalue));

    printf("run_mode:%s\n", s_param_list[i].run_mode);

    jvalue = json_object_object_get(jobj, "run_times");
    s_param_list[i].run_times = json_object_get_int(jvalue);

    printf("run_times:%d\n", s_param_list[i].run_times);

    jvalue = json_object_object_get(jobj, "inference_times");
    s_param_list[i].inference_times = json_object_get_int(jvalue);

    printf("inference_times:%d\n", s_param_list[i].inference_times);
     
    jvalue = json_object_object_get(jobj, "dev_idx");
    s_param_list[i].dev_idx = json_object_get_int(jvalue);

    printf("dev_idx:%d\n", s_param_list[i].dev_idx);

    jvalue = json_object_object_get(jobj, "ion");
    s_param_list[i].ion = json_object_get_int(jvalue);
    printf("ion:%d\n", s_param_list[i].ion);

    jvalue = json_object_object_get(jobj, "class_num");
    s_param_list[i].class_num = json_object_get_int(jvalue);

    printf("class_num:%d\n", s_param_list[i].class_num);
  }

  free(buf);
  printf("----config parser done-------\n");

  return 0;
}
 
//============================ Load Test Files =======================//
int load_test_bin(const char* path, unsigned int* binLength, char** testBin)
{
  FILE* fp;
  char* buf;
  unsigned int length = 0;

  fp = fopen(path, "rb");
  if(fp == NULL)
  {
    printf("load_test_bin failed, open file failed\n");
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  rewind(fp);

  if(!(buf = (char*)malloc(length)))
  {
    fclose(fp);
    return -1;
  }

  if(fread(buf, 1, length, fp) != length)
  {
    fclose(fp);
    free(buf);
    printf("load_test_bin failed, read length is not right\n");
    return -1;
  }

  fclose(fp);

  *testBin = buf;
  *binLength = length;

  return 0;
}


int load_graph_file(const char* path, char** graphFile, unsigned int* graphSize) {
  FILE* fp;
  char* buf;
  unsigned int length;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    printf("[%s] open graph file failed.\n", __func__);
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  rewind(fp);

  if (!(buf = (char*)malloc(length))) {
    printf("[%s] malloc buf failed.\n", __func__);
    fclose(fp);
    return -1;
  }
  if (fread(buf, 1, length, fp) != length) {
    printf("[%s] read length mismatch.\n", __func__);
    fclose(fp);
    free(buf);
    return -1;
  }
  fclose(fp);

  *graphFile = buf;
  *graphSize = length;
  
  return 0;
}


int write_output_file(const char* path, char* buf, unsigned int length) {
  FILE* fp;

  printf("write_output_file path is:%s\n", path);
  fp = fopen(path, "wb");
  if (fp == NULL)
    return 0;

  if (fwrite(buf, 1, length, fp) != length) {
    fclose(fp);
    return -1;
  }
  fclose(fp);
  return 0;
}