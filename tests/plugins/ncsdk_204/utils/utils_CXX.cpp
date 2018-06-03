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
#include <ion_kernel.h>
#include <ion.h>

#include "testcase.h"

//======================= variables =======================//
size_t align = 0;
int prot = PROT_READ | PROT_WRITE;
int map_flags = MAP_SHARED;
int alloc_flags = 0;
int heap_mask = ION_DMA_HEAP_ID;

//======================= Basic Functions =================//
void usage(char* n) {
  printf("Usage:\n");
  printf("  %s <vaname> <connectCode>\n", n);
  printf("Example:\n");
  printf("  %s car0 123\n", n);
}

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
int load_cfg(const char* file_path, void* param) {
  FILE* fp;
  char* buf;
  int i = 0;

  NVA_PARAM* s_param_list = (NVA_PARAM*)param;

  fp = fopen(file_path, "r");
  if (fp == NULL)
    return -1;
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

    struct json_object* jvalue = json_object_object_get(jobj, "connectcode");
    strcpy((char*)&s_param_list[i].connectcode, json_object_get_string(jvalue));

    printf("connectcode:%s\n", s_param_list[i].connectcode);

    jvalue = json_object_object_get(jobj, "vaname");
    strcpy((char*)&s_param_list[i].vaname, json_object_get_string(jvalue));

    printf("vaname:%s\n", s_param_list[i].vaname);

    jvalue = json_object_object_get(jobj, "tensor");
    strcpy((char*)&s_param_list[i].tensor, json_object_get_string(jvalue));

    printf("tensor:%s\n", s_param_list[i].tensor);

    jvalue = json_object_object_get(jobj, "graph_path");
    strcpy((char*)&s_param_list[i].graph_path, json_object_get_string(jvalue));

    printf("graph_path:%s\n", s_param_list[i].graph_path);

    jvalue = json_object_object_get(jobj, "testfile_path");
    strcpy((char*)&s_param_list[i].testfile_path, json_object_get_string(jvalue));

    printf("testfile_path:%s\n", s_param_list[i].testfile_path);

    jvalue = json_object_object_get(jobj, "sw");
    strcpy((char*)&s_param_list[i].sw, json_object_get_string(jvalue));

    printf("sw:%s\n", s_param_list[i].sw);

    jvalue = json_object_object_get(jobj, "to_format");
    strcpy((char*)&s_param_list[i].to_format_str,
           json_object_get_string(jvalue));

    printf("to_format:%s\n", s_param_list[i].to_format_str);
    if (strcasecmp(s_param_list[i].to_format_str, "nv12") == 0) {
      s_param_list[i].to_format = MFX_MAKEFOURCC('N', 'V', '1', '2');
    } else if (strcasecmp(s_param_list[i].to_format_str, "rgb4") == 0) {
      s_param_list[i].to_format = MFX_MAKEFOURCC('R', 'G', 'B', '4');
    } else if (strcasecmp(s_param_list[i].to_format_str, "yuy2") == 0) {
      s_param_list[i].to_format = MFX_MAKEFOURCC('Y', 'U', 'Y', '2');
    }

    jvalue = json_object_object_get(jobj, "dev_idx");
    s_param_list[i].dev_idx = json_object_get_int(jvalue);

    printf("dev_idx:%d\n", s_param_list[i].dev_idx);

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


int load_test_bin_ion(const char* path, unsigned int* binLength, int* shareFd)
{
  int ion_fd = ion_open();
  FILE* fp;
  char* buf;

  unsigned int length = 0;
  int share_fd = -1;

  int ret;
  ion_phys_addr_t phys_addr,cpu_addr;
  ion_user_handle_t handle;

//Get Alloc size   
  fp = fopen(path, "rb");
  if(fp == NULL){
    printf("load_test_bin failed, open file failed\n");
    return -1;
  }

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  rewind(fp);

//Alloc memory and share 
  ret = ion_alloc(ion_fd, length, align, heap_mask, alloc_flags, &handle, &phys_addr, &cpu_addr);
  if(ret<0)
  {
    printf("\033[41;36m  ion_alloc  failed, ret=%d \033[0m\n",ret);
    return -1;
  }

  ret = ion_share(ion_fd, handle, &share_fd);
  if(ret<0)
  {
    printf("\033[41;36m  ion_share  failed, ret=%d \033[0m\n",ret);
    return -1;
  }

  ret = ion_free(ion_fd, handle);
  if(ret<0)
  {
    printf("\033[41;36m  ion_free failed, ret=%d \033[0m\n",ret);
    return -1;
  }

//begin: Write to this memory take advatage of shared_fd
  buf = (char*)mmap(NULL, length, prot, map_flags, share_fd, 0);

  if(fread(buf, 1, length, fp) != length)
  {
    fclose(fp);
    free(buf);
    printf("load_test_bin failed, read length is not right\n");
    return -1;
  }
  munmap(buf, length);
//end

  fclose(fp);

  *binLength = length;
  *shareFd = share_fd;
  ion_close(ion_fd);
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