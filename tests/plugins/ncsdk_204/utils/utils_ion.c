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

#include <ion_kernel.h>
#include <ion.h>


//======================= variables =======================//
size_t align = 0;
int prot = PROT_READ | PROT_WRITE;
int map_flags = MAP_SHARED;
int alloc_flags = 0;
int heap_mask = ION_DMA_HEAP_ID;

  

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
    printf("load_test_bin_ion failed, open file failed\n");
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
    printf("load_test_bin_ion failed, read length is not right\n");
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