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

#ifndef __SYS_CORE_ION_H
#define __SYS_CORE_ION_H

#include <sys/types.h>
#include "ion_kernel.h"

__BEGIN_DECLS

struct ion_handle;

int ion_open();
int ion_close(int fd);

//heap_mask is heap id mask
// ION_DMA_HEAP_ID ==31
/*
  if (buffer->flags & ION_FLAG_CACHED)
    return -EINVAL;

  if (align > PAGE_SIZE)
    return -EINVAL;

*/

#ifndef DEBUG_ION
int ion_alloc_only(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle,
              ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr);

#define ion_alloc(fd,len,align,heap_mask,flags,handle,phys_addr,cpu_addr)\
		ion_alloc_only(fd,len,align,heap_mask, flags, handle, phys_addr, cpu_addr)


#else
int ion_alloc_debug(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, ion_user_handle_t *handle,
              ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr, int linenum, const char* func);

#define ion_alloc(fd,len,align,heap_mask,flags,handle,phys_addr,cpu_addr)\
		ion_alloc_debug(fd,len,align,heap_mask, flags, handle, phys_addr, cpu_addr, __LINE__, __func__)

#endif


int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
              unsigned int flags, int *handle_fd,ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr);

int ion_sync_fd(int fd, int handle_fd);
int ion_free(int fd, ion_user_handle_t handle);
int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd);
int ion_share(int fd, ion_user_handle_t handle, int *share_fd);
int ion_import(int fd, int share_fd, ion_user_handle_t *handle);

/**
  * Add 4.12+ kernel ION interfaces here for forward compatibility
  * This should be needed till the pre-4.12+ ION interfaces are backported.
  */
/*
int ion_query_heap_cnt(int fd, int* cnt);
int ion_query_get_heaps(int fd, int cnt, void* buffers);
*/
int ion_is_legacy(int fd);

int ion_get_info(int fd, ion_user_handle_t handle,ion_phys_addr_t* phys_addr,ion_cpu_addr_t* cpu_addr,size_t* size);

int ion_query_max_alloc_size(int fd, int* max_size);

__END_DECLS

#endif /* __SYS_CORE_ION_H */
