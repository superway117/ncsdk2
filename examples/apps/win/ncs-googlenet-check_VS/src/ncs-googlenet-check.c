// Copyright 2018 Intel Corporation.
// The source code, information and material ("Material") contained herein is  
// owned by Intel Corporation or its suppliers or licensors, and title to such  
// Material remains with Intel Corporation or its suppliers or licensors.  
// The Material contains proprietary information of Intel or its suppliers and  
// licensors. The Material is protected by worldwide copyright laws and treaty  
// provisions.  
// No part of the Material may be used, copied, reproduced, modified, published,  
// uploaded, posted, transmitted, distributed or disclosed in any way without  
// Intel's prior express written permission. No license under any patent,  
// copyright or other intellectual property rights in the Material is granted to  
// or conferred upon you, either expressly, by implication, inducement, estoppel  
// or otherwise.  
// Any license under such intellectual property rights must be express and  
// approved by Intel in writing. 


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mvnc.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#if (defined(_WIN32) || defined(_WIN64) )
#include "gettime.h"
#define strcasecmp _stricmp
#endif

// GoogleNet image dimensions, network mean values for each channel in BGR order.
const int networkDim = 224;
float networkMean[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

typedef unsigned short half;

double seconds()
{
    static double s;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if(!s)
        s = ts.tv_sec + ts.tv_nsec * 1e-9;
    return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}

void *loadfile(const char *path, unsigned int *length)
{
    FILE *fp;
    char *buf;

    fp = fopen(path, "rb");
    if(fp == NULL)
        return 0;
    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    rewind(fp);
    if(!(buf = malloc(*length)))
    {
        fclose(fp);
        return 0;
    }
    if(fread(buf, 1, *length, fp) != *length)
    {
        fclose(fp);
        free(buf);
        return 0;
    }
    fclose(fp);
    return buf;
}

static float *cmpdata;
int indexcmp(const void *a1, const void *b1)
{
    int *a = (int *)a1;
    int *b = (int *)b1;
    float diff = cmpdata[*b] - cmpdata[*a];
    if(diff < 0)
        return -1;
    else if(diff > 0)
        return 1;
    else return 0;
}

int loadgraphdata(const char *dir, int *reqsize, float *mean, float *std)
{
    char path[300];
    int i;

    snprintf(path, sizeof(path), "%s/stat.txt", dir);
    FILE *fp = fopen(path, "r");
    if(!fp)
    {
        perror(path);
        return -1;
    }
    if(fscanf(fp, "%f %f %f\n%f %f %f\n", mean, mean+1, mean+2, std, std+1, std+2) != 6)
    {
        printf("%s: mean and stddev not found in file\n", path);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    for(i = 0; i < 3; i++)
    {
        mean[i] = 255.0 * mean[i];
        std[i] = 1.0 / (255.0 * std[i]);
    }
    snprintf(path, sizeof(path), "%s/inputsize.txt", dir);
    fp = fopen(path, "r");
    if(!fp)
    {
        perror(path);
        return -1;
    }
    if(fscanf(fp, "%d", reqsize) != 1)
    {
        printf("%s: inputsize not found in file\n", path);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

float *loadimage(const char *path, int reqsize, float *mean, unsigned int* imgsize)
{
    int width, height, cp, i;
    unsigned char *img, *imgresized;
    float *imgfp32;

    img = stbi_load(path, &width, &height, &cp, 3);
    if(!img)
    {
        printf("The picture %s could not be loaded\n", path);
        return 0;
    }
    imgresized = malloc(3*reqsize*reqsize);
    if(!imgresized)
    {
        free(img);
        perror("malloc");
        return 0;
    }
    stbir_resize_uint8(img, width, height, 0, imgresized, reqsize, reqsize, 0, 3);
    free(img);
    *imgsize = sizeof(*imgfp32) * reqsize * reqsize * 3;
    imgfp32 = malloc(*imgsize);
    if(!imgfp32)
    {
        free(imgresized);
        perror("malloc");
        return 0;
    }
    for(i = 0; i < reqsize * reqsize * 3; i++)
        imgfp32[i] = imgresized[i];
    free(imgresized);
    for(i = 0; i < reqsize*reqsize; i++)
    {
        imgfp32[3*i] = imgfp32[3*i]-mean[0];
        imgfp32[3*i+1] = imgfp32[3*i+1]-mean[1];
        imgfp32[3*i+2] = imgfp32[3*i+2]-mean[2];
    }
    return imgfp32;
}

void runinference(struct ncGraphHandle_t *graph, struct ncDeviceHandle_t *dev,
                  struct ncFifoHandle_t * bufferIn, struct ncFifoHandle_t * bufferOut,
                  const char *path, char **categories, int reqsize, float *mean)
{
    int i;
    void *userParam;
    float *timetaken;
    unsigned int timetakenlen;
    struct ncTensorDescriptor_t td;
    unsigned int length;
    int *indexes;
    float *img;
    double t;
    unsigned int imgsize;

    // Load image from file & preprocess
    img = loadimage(path, reqsize, mean, &imgsize);
    if (!img) 
    {
        return;
    }
    // Read descriptor for input tensor
    length = sizeof(td);
    int rc = ncFifoGetOption(bufferIn, NC_RO_FIFO_TENSOR_DESCRIPTOR, &td, &length);
    if (rc || length != sizeof(td)){
        printf("ncFifoGetOption failed, rc=%d\n", rc);
        return;
    }

    t = seconds();

    // Write tensor to input fifo
    // It is not mandatory to give the tensor descriptor.  If the pointer is NULL, the implementation will default to the structure of the FIFO
    rc = ncFifoWriteElem(bufferIn, img, &imgsize, 0);
    if(rc)
    {
        free(img);
        printf("ncFifoWriteElem failed, rc=%d\n", rc);
        return;
    }

    // Start inference
    rc = ncGraphQueueInference(graph, &bufferIn, 1, &bufferOut, 1);
    if(rc)
    {
        free(img);
        printf("ncGraphQueueInference failed, rc=%d\n", rc);
        return;
    }
    free(img);

    // Read output results
    unsigned int outputDataLength;
    length = sizeof(unsigned int);
    rc = ncFifoGetOption(bufferOut, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (rc || length != sizeof(unsigned int)){
        printf("ncFifoGetOption failed, rc=%d\n", rc);
        return;
    }
    void *result = malloc(outputDataLength);
    if (!result) {
        printf("malloc failed!\n");
        return;
    }
    rc = ncFifoReadElem(bufferOut, result, &outputDataLength, &userParam);
    if(rc)
    {
        if(rc == NC_MYRIAD_ERROR)
        {
            char debuginfo[NC_DEBUG_BUFFER_SIZE];
            unsigned debuginfolen = NC_DEBUG_BUFFER_SIZE;
            rc = ncGraphGetOption(graph, NC_RO_GRAPH_DEBUG_INFO, (void *)debuginfo, &debuginfolen);
            if(rc == 0)
            {
                printf("ncGraphGetOption failed, myriad error: %s\n", debuginfo);
                return;
            }
        }
        printf("ncFifoReadElem failed, rc=%d\n", rc);
        return;
    }
    //printf("Returned %u bytes of inference result\n", resultDesc.totalSize);

    // Get results
    unsigned int resultlen = outputDataLength / sizeof(float);
    indexes = malloc(sizeof(*indexes) * resultlen);
    for(i = 0; i < (int)resultlen; i++)
        indexes[i] = i;
    cmpdata = (float*)result;
    qsort(indexes, resultlen, sizeof(*indexes), indexcmp);

    // Print Top5 results
    printf("\nTop5 results\n-------------\n");
    for(i = 0; i < 5 && i < resultlen; i++) {
        printf("%s (%.2f%%)\n", categories[indexes[i]], cmpdata[indexes[i]] * 100.0);
    }
    printf("\n");

    free(indexes);

    // Get inference time
    int time_taken_array_size;
    rc = ncGraphGetOption(graph, NC_RO_GRAPH_TIME_TAKEN_ARRAY_SIZE, &time_taken_array_size, &length);
    if (rc || time_taken_array_size < sizeof(float) || time_taken_array_size%sizeof(float) != 0) {
        printf("Failed reading time taken array size - rc %d\n", rc);
        printf("Test failed :(\n");
        return;
    }

    timetaken = (float*) malloc(time_taken_array_size);
    timetakenlen = time_taken_array_size;
    rc = ncGraphGetOption(graph, NC_RO_GRAPH_TIME_TAKEN, (void *)timetaken, &timetakenlen);
    if(rc)
    {
        printf("ncGraphGetOption failed, rc=%d\n", rc);
        free(timetaken);
        return;
    }
    timetakenlen = timetakenlen / sizeof(*timetaken);
    float sum = 0;
    for(i = 0; i < timetakenlen; i++)
        sum += timetaken[i];
    printf("Inference time: %f ms, total time %f ms\n", sum, (seconds() - t) * 1000.0);

    free(result);
    free(timetaken);
}

int help()
{
    fprintf(stderr, "./ncs-googlenet-check [-l<loglevel>] [[-n<count>] -g<graph file> -c<categories file> -i<image>]\n");
    fprintf(stderr, "                <loglevel> API log level\n");
    fprintf(stderr, "                <count> is the number of inference iterations, default 2\n");
    fprintf(stderr, "                <graph file> path to GoogleNet graph file\n");
    fprintf(stderr, "                <categories file> path to categories file\n");
    fprintf(stderr, "                if graph is not provided, app will just open and close the NCS device\n");
    return 0;
}

static char **categories;
static int ncategories;

int loadcategories(const char *path)
{
    char line[300], *p;
    FILE *fp = fopen(path, "r");
    if(!fp)
    {
        perror(path);
        return -1;
    }
    ncategories = 0;
    categories = malloc(1000 * sizeof(*categories));
    while(fgets(line, sizeof(line), fp))
    {
        p = strchr(line, '\n');
        if(p)
            // Handle windows line endings
            if(*(p-1) == '\r')
                *(p-1) = 0;
            *p = 0;
        if(strcasecmp(line, "classes"))
        {
            p = strchr(line, ' ');
            if (p != NULL) {
                p++;
                categories[ncategories++] = strdup(p);
                if(ncategories == 1000)
                    break;
            }
        }
    }
    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    int rc, i;
    struct ncGraphHandle_t *g;
    struct ncDeviceHandle_t *h;
    int loglevel = -1, inference_count = 2;
    char* picture = NULL;
    void *graphfile = NULL;
    char* graph_path = NULL;
    char* categories_path = NULL;
    unsigned int graphsize, length;

    // Parse command line arguments
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            if (argc <= i+1) {
                printf("Error: missing argument for option %s\n", argv[i]);
                return help();
            }
            if (argv[i][1] == 'l')
                loglevel = atoi(argv[++i]);
            else if (argv[i][1] == 'n')
                inference_count = atoi(argv[++i]);
            else if (argv[i][1] == 'c')
                categories_path = strdup(argv[++i]);
            else if (argv[i][1] == 'g')
                graph_path = strdup(argv[++i]);
            else if (argv[i][1] == 'i')
                picture = strdup(argv[++i]);
            else {
                printf("Error: unsupported option %s\n", argv[i]);
                return help();
            }
        } 
    }

    // Set log level
    if (loglevel != -1)
        ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    // Initialize device handle
    rc = ncDeviceCreate(0, &h);
    if(rc != NC_OK)
    {
        printf("Error - No NCS devices found.\n");
        printf("    ncStatus value: %d\n", rc);
        exit(-1);
    }

    // Open device
    if( (rc=ncDeviceOpen(h) ))
    {
        printf("ncDeviceOpen failed, rc=%d\n", rc);
        exit(-1);
    }
    printf("Successfully opened NCS device!\n");
    if (graph_path)
    {
        graphfile = loadfile(graph_path, &graphsize);
    }
    if(graphfile)
    {
        if(!loadcategories(categories_path))
        {
            unsigned int mem, memMax;

            // Read device memory info
            length = sizeof(unsigned int);
            rc = ncDeviceGetOption(h, NC_RO_DEVICE_CURRENT_MEMORY_USED, (void **)&mem, &length);
            rc += ncDeviceGetOption(h, NC_RO_DEVICE_MEMORY_SIZE, (void **)&memMax, &length);
            if(rc)
            {
                printf("ncDeviceGetOption failed, rc=%d\n", rc);
            }
            else
            {
                printf("Current memory used on device is %d out of %d\n", mem, memMax);
            }

            // Init graph handle
            rc = ncGraphCreate("firstGraph", &g);
            if(rc != NC_OK)
            {
                printf("ncGraphCreate failed, rc=%d\n", rc);
                exit(-1);
            }

            // Send graph to device
            rc = ncGraphAllocate(h, g, graphfile, graphsize);
            if(rc == NC_OK)
            {
                // Read tensor descriptors
                struct ncTensorDescriptor_t inputTensorDesc;
                struct ncTensorDescriptor_t outputTensorDesc;
                length = sizeof(struct ncTensorDescriptor_t);
                ncGraphGetOption(g, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTensorDesc,  &length);
                ncGraphGetOption(g, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTensorDesc,  &length);

                // Init & Create Fifos
                struct ncFifoHandle_t * bufferIn;
                struct ncFifoHandle_t * bufferOut;
                rc = ncFifoCreate("FifoIn0", NC_FIFO_HOST_WO, &bufferIn);
                if(rc != NC_OK) 
                {
                    printf("Error - Input Fifo creation failed!");
                    exit(-1);
                }
                rc = ncFifoAllocate(bufferIn, h, &inputTensorDesc, 2);
                if(rc != NC_OK) 
                {
                    printf("Error - Input Fifo allocation failed!");
                    exit(-1);
                }
                rc = ncFifoCreate("FifoOut0", NC_FIFO_HOST_RO, &bufferOut);
                if(rc != NC_OK) 
                {
                    printf("Error - Output Fifo creation failed!");
                    exit(-1);
                }
                rc = ncFifoAllocate(bufferOut, h, &outputTensorDesc, 2);
                if(rc != NC_OK) 
                {
                    printf("Error - Output Fifo allocation failed!");
                    exit(-1);
                }

                // Run inferences
                for(i = 0; i < inference_count; i++){
                    runinference(g, h, bufferIn, bufferOut, picture, categories, networkDim, networkMean);
                }

                // Deallocate graph and fifos
                rc = ncFifoDestroy(&bufferIn);
                if (rc != NC_OK)
                {
                    printf("Error - Failed to deallocate input fifo!");
                    exit(-1);
                }          
                rc = ncFifoDestroy(&bufferOut);
                if (rc != NC_OK)
                {
                    printf("Error - Failed to deallocate output fifo!");
                    exit(-1);
                }          
                rc = ncGraphDestroy(&g);
                if (rc != NC_OK)
                {
                    printf("Error - Failed to deallocate graph!");
                    exit(-1);
                }
            }
            else
            {
                printf("Could not allocate graph file: %s\n", graph_path); 
                printf("Error from ncGraphAllocate is: %d\n", rc);
            }
            // Cleanup
            for(i = 0; i < ncategories; i++)
                free(categories[i]);
            free(categories);
        }
        
        free(graphfile);
    }
    else
    {
        if (graph_path)
            fprintf(stderr, "%s not found\n", graph_path);
        else
            printf("Network was not provided, closing device...\n");
    }
    // Close device
    rc = ncDeviceClose(h);
    if(rc)
        printf("ncDeviceClose failed, rc=%d\n", rc);
    else
        printf("Goodbye NCS!  Device Closed normally.\n");

    return 0;
}

