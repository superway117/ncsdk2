#ifndef __UTILS_H__
#define __UTILS_H__

extern char* voc_names[];
extern char* barrier_names[];

void getTime(struct timespec nowTime);
int load_cfg(const char* file_path, void* param);
int load_test_runner_cfg(const char* file_path, void* param);
int load_test_bin(const char* path, unsigned int* binLength, char** testBin);
int load_graph_file(const char* path, char** graphFile, unsigned int* graphSize);
int write_output_file(const char* path, char* buf, unsigned int length);

#endif