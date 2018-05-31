/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#define LOG_TAG "ion"

#include <errno.h>
#include <fcntl.h>
//#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include "ion_kernel.h"
#include "ion.h"
#include "ion_update.h"

enum ion_version { ION_VERSION_UNKNOWN, ION_VERSION_MODERN, ION_VERSION_LEGACY };

//static atomic_int g_ion_version = ATOMIC_VAR_INIT(ION_VERSION_UNKNOWN);

int ion_is_legacy(int fd) {

    /*
    int version = atomic_load_explicit(&g_ion_version, memory_order_acquire);
    if (version == ION_VERSION_UNKNOWN) {
        */
        /**
          * Check for FREE IOCTL here; it is available only in the old
          * kernels, not the new ones.
          *//**
        int err = ion_free(fd, (ion_user_handle_t)((intptr_t) NULL));
        version = (err == -ENOTTY) ? ION_VERSION_MODERN : ION_VERSION_LEGACY;
        atomic_store_explicit(&g_ion_version, version, memory_order_release);
    }
    return version == ION_VERSION_LEGACY;*/
    return 1;
}

int ion_open() {

    int fd = open("/dev/ion", O_RDWR | O_CLOEXEC);
    if (fd < 0) printf("open /dev/ion failed!\n");

    return fd;
}

int ion_close(int fd) {

    int ret = close(fd);
    if (ret < 0) return -errno;
    return ret;
}

static int ion_ioctl(int fd, int req, void *arg) {
    int ret = ioctl(fd, req, arg);
    if (ret < 0) {
        printf("ioctl %x failed with code %d: %s\n", req, ret, strerror(errno));
        return -errno;
    }
    return ret;
}

int ion_alloc_only(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle,
              ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr) {
    int ret = 0;

    if ((handle == NULL) || (!ion_is_legacy(fd))) return -EINVAL;

    struct ion_allocation_data data = {
        .len = len,
        .align = align,
        .heap_id_mask = heap_mask,
        .flags = flags,
        /*Added for debug by jiayi*/
        .alloc_linenum = __LINE__,
    };
    /*Added for debug by jiayi*/
    memcpy(data.alloc_caller, __func__, sizeof(char)*MAX_FUNC_NAME_LEN);

    ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
    if (ret < 0) {
        printf("ion alloc failed size=%d\n",(int)len);
        return ret;
    }
    //printf("ion alloc succ size=%d\n",(int)len);
    *phys_addr = data.phys_addr;
    *cpu_addr = data.cpu_addr;
    *handle = data.handle;
    return ret;
}

int ion_alloc_debug(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle,
              ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr, int linenum,const char* func) {
    int ret = 0;

    if ((handle == NULL) || (!ion_is_legacy(fd))) return -EINVAL;

    struct ion_allocation_data data = {
        .len = len,
        .align = align,
        .heap_id_mask = heap_mask,
        .flags = flags,
        /*Added for debug by jiayi*/
        .alloc_linenum = linenum,
    };
    /*Added for debug by jiayi*/
    memcpy(data.alloc_caller, func, sizeof(char)*MAX_FUNC_NAME_LEN);

    ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
    if (ret < 0)
    {
        printf("ion alloc failed size=%d\n",(int)len);
        return ret;
    }
    //printf("ion alloc succ size=%d\n",(int)len);
    *phys_addr = data.phys_addr;
    *cpu_addr = data.cpu_addr;
    *handle = data.handle;

    return ret;
}

int ion_free(int fd, ion_user_handle_t handle) {
    struct ion_handle_data data = {
        .handle = handle,
    };
    int ret = ion_ioctl(fd, ION_IOC_FREE, &data);
    if (ret < 0)
    {
        printf("ion free failed\n");
        return ret;
    }
    return ret;
}

int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot, int flags, off_t offset,
            unsigned char** ptr, int* map_fd) {
    if (!ion_is_legacy(fd)) return -EINVAL;
    int ret;
    unsigned char* tmp_ptr;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (map_fd == NULL) return -EINVAL;
    if (ptr == NULL) return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_MAP, &data);
    if (ret < 0) return ret;
    if (data.fd < 0) {
        printf("map ioctl returned negative fd\n");
        return -EINVAL;
    }
    tmp_ptr = mmap(NULL, length, prot, flags, data.fd, offset);
    if (tmp_ptr == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        return -errno;
    }
    *map_fd = data.fd;
    *ptr = tmp_ptr;
    return ret;
}

int ion_share(int fd, ion_user_handle_t handle, int* share_fd) {
    int ret;
    struct ion_fd_data data = {
        .handle = handle,
    };

    if (!ion_is_legacy(fd)) return -EINVAL;
    if (share_fd == NULL) return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
    if (ret < 0) return ret;
    if (data.fd < 0) {
        printf("share ioctl returned negative fd\n");
        return -EINVAL;
    }
    *share_fd = data.fd;
    return ret;
}

int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,unsigned int flags,
                 int* handle_fd,
                 ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr) {
    ion_user_handle_t handle;
    int ret;

    if (!ion_is_legacy(fd)) {
        struct ion_new_allocation_data data = {
            .len = len,
            .heap_id_mask = heap_mask,
            .flags = flags,
            .alloc_linenum = __LINE__,
        };
	memcpy(data.alloc_caller, __func__, sizeof(char)*MAX_FUNC_NAME_LEN);

        ret = ion_ioctl(fd, ION_IOC_NEW_ALLOC, &data);
        if (ret < 0) return ret;
        *handle_fd = data.fd;
    } else {
        ret = ion_alloc(fd, len, align, heap_mask, flags, &handle, phys_addr, cpu_addr);
        if (ret < 0) return ret;
        ret = ion_share(fd, handle, handle_fd);
        ion_free(fd, handle);
    }
    return ret;
}

int ion_import(int fd, int share_fd, ion_user_handle_t* handle) {
    int ret;
    struct ion_fd_data data = {
        .fd = share_fd,
    };

    if (!ion_is_legacy(fd)) return -EINVAL;

    if (handle == NULL) return -EINVAL;

    ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
    if (ret < 0) return ret;
    *handle = data.handle;
    return ret;
}

int ion_sync_fd(int fd, int handle_fd) {
    struct ion_fd_data data = {
        .fd = handle_fd,
    };

    if (!ion_is_legacy(fd)) return -EINVAL;

    return ion_ioctl(fd, ION_IOC_SYNC, &data);
}

/*
int ion_query_heap_cnt(int fd, int* cnt) {
    int ret;
    struct ion_heap_query query;

    memset(&query, 0, sizeof(query));

    ret = ion_ioctl(fd, ION_IOC_HEAP_QUERY, &query);
    if (ret < 0) return ret;

    *cnt = query.cnt;
    return ret;
}

int ion_query_get_heaps(int fd, int cnt, void* buffers) {
    int ret;
    struct ion_heap_query query = {
        .cnt = cnt, .heaps = (uintptr_t)buffers,
    };

    ret = ion_ioctl(fd, ION_IOC_HEAP_QUERY, &query);
    return ret;
}
*/
int ion_get_info(int fd, ion_user_handle_t handle,ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr,size_t* size) {
    struct ion_info_data data = {
        .handle = handle,
    };
    int ret = ion_ioctl(fd, ION_IOC_GET_INFO, &data);
    if(ret<0)
    {
        printf("ion get info failed ret=%d\n",ret);
        return ret;
    }

    *phys_addr = data.phys_addr;
    *cpu_addr = data.cpu_addr;
    *size = data.len;
    return 0;
}

int ion_query_max_alloc_size(int fd, int* max_size) {
    struct ion_query_max_order_data data;
    int ret = ion_ioctl(fd, ION_IOC_QUERY_MAX_ORDER, &data);
    if(ret<0)
    {
        printf("ion query max order failed, ret=%d\n", ret);
    }
    *max_size = pow(2,data.max_order)*1024; //Max allocate size: Byte
    return 0;
}
