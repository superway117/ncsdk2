#ifndef __UTILS_ION_H__
#define __UTILS_ION_H__
 
extern size_t align;
extern int prot;
extern int map_flags;
extern int alloc_flags;
extern int heap_mask;

int load_test_bin_ion(const char* path, unsigned int* binLength, int* shareFd);

#endif