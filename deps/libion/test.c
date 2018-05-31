/*
 *   Copyright 2013 Google, Inc
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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ion_kernel.h"
#include "ion.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)

size_t len = 1024*1024, align = 0;
int prot = PROT_READ | PROT_WRITE;
int map_flags = MAP_SHARED;
int alloc_flags = 0;
int heap_mask = ION_DMA_HEAP_ID;
int test = -1;
size_t stride;

int _ion_alloc_test(int *fd, ion_user_handle_t *handle)
{
    int ret;
    ion_phys_addr_t phys_addr,cpu_addr;
    *fd = ion_open();
    if (*fd < 0)
        return *fd;

    ret = ion_alloc(*fd, len, align, heap_mask, alloc_flags, handle,&phys_addr,&cpu_addr);

    if (ret)
        printf("%s failed: %s\n", __func__, strerror(ret));
    else
        printf("%s alloc succ: %p,%p\n", __func__, (void*)phys_addr,(void*)cpu_addr);
    return ret;
}

void ion_alloc_test()
{
    int fd, ret;
    ion_user_handle_t handle;

    if(_ion_alloc_test(&fd, &handle))
        return;

    ret = ion_free(fd, handle);
    if (ret) {
        printf("%s failed: %s %d\n", __func__, strerror(ret), handle);
        return;
    }
    ion_close(fd);
    printf("ion alloc test: passed\n");
}

void ion_get_info_get()
{
    int fd, ret;
    ion_user_handle_t handle;

    if(_ion_alloc_test(&fd, &handle))
        return;

    ion_phys_addr_t phys_addr;
    ion_cpu_addr_t cpu_addr;
    size_t size;
    ret = ion_get_info(fd, handle,&phys_addr,&cpu_addr,&size);
    if (ret) {
        printf("%s ion_get_info failed: %s %d\n", __func__, strerror(ret), handle);
        return;
    }

    ret = ion_free(fd, handle);
    if (ret) {
        printf("%s failed: %s %d\n", __func__, strerror(ret), handle);
        return;
    }
    ion_close(fd);
    printf("ion get info test: passed\n");
}


void ion_alloc_many_buf_test()
{
    int fd, ret,i;
    ion_user_handle_t handle[100];

    ion_phys_addr_t phys_addr,cpu_addr;
    int alloc_num = 0;
    int free_num = 0;
    fd = ion_open();
    if (fd < 0)
    {
        printf("%s open device failed\n", __func__);
        return;
    }

    for(i=0;i<100;i++)
    {
        ret = ion_alloc(fd, len, align, heap_mask, alloc_flags, &handle[i],&phys_addr,&cpu_addr);


        if(ret)
        {
            printf("%s alloc failed: %s %d\n", __func__, strerror(ret), handle[i]);
            break;
        }
        alloc_num++;
    }
    for(i=0;i<100;i++)
    {
        ret = ion_free(fd, handle[i]);
        if (ret) {
            printf("%s free failed: %s %d\n", __func__, strerror(ret), handle[i]);
            break;
        }
        free_num++;

    }
    ion_close(fd);
    printf("%s alloc times:%d, free times: %d\n",__func__,alloc_num,free_num);
}

void ion_map_test()
{
    int fd, map_fd, ret;
    size_t i;
    ion_user_handle_t handle;
    unsigned char *ptr;

    if(_ion_alloc_test(&fd, &handle))
        return;

    ret = ion_map(fd, handle, len, prot, map_flags, 0, &ptr, &map_fd);
    if (ret)
        return;

    for (i = 0; i < len; i++) {
        ptr[i] = (unsigned char)i;
    }
    for (i = 0; i < len; i++)
        if (ptr[i] != (unsigned char)i)
            printf("%s failed wrote %zu read %d from mapped "
                   "memory\n", __func__, i, ptr[i]);
    /* clean up properly */
    ret = ion_free(fd, handle);
    ion_close(fd);
    munmap(ptr, len);
    close(map_fd);

    _ion_alloc_test(&fd, &handle);
    close(fd);

#if 0
    munmap(ptr, len);
    close(map_fd);
    ion_close(fd);

    _ion_alloc_test(len, align, flags, &fd, &handle);
    close(map_fd);
    ret = ion_map(fd, handle, len, prot, flags, 0, &ptr, &map_fd);
    /* don't clean up */
#endif
}

void ion_share_test()

{
    ion_user_handle_t handle;
    int sd[2];
    int num_fd = 1;
    struct iovec count_vec = {
        .iov_base = &num_fd,
        .iov_len = sizeof num_fd,
    };
    char buf[CMSG_SPACE(sizeof(int))];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sd);
    if (fork()) {
        struct msghdr msg = {
            .msg_control = buf,
            .msg_controllen = sizeof buf,
            .msg_iov = &count_vec,
            .msg_iovlen = 1,
        };

        struct cmsghdr *cmsg;
        int fd, share_fd, ret;
        char *ptr;
        /* parent */
        if(_ion_alloc_test(&fd, &handle))
            return;
        ret = ion_share(fd, handle, &share_fd);
        if (ret)
            printf("share failed %s\n", strerror(errno));
        ptr = mmap(NULL, len, prot, map_flags, share_fd, 0);
        if (ptr == MAP_FAILED) {
            return;
        }
        strcpy(ptr, "master");
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        *(int *)CMSG_DATA(cmsg) = share_fd;
        /* send the fd */
        printf("[master] prepare sending msg to child, content [%10s] should be `master`\n", ptr);

        sendmsg(sd[0], &msg, 0);
        if (recvmsg(sd[0], &msg, 0) < 0)
            perror("[master] recv msg from child");
        printf("[master] recv msg from child, content is [%10s] , it should be `child`\n", ptr);

        /* send ping */
        sendmsg(sd[0], &msg, 0);
        printf("master->master? [%10s]\n", ptr);
        if (recvmsg(sd[0], &msg, 0) < 0)
            perror("master recv 1");
    } else {
        //struct msghdr msg;
        struct cmsghdr *cmsg;
        char* ptr;
        int  recv_fd;
        char* child_buf[100];
        /* child */
        struct iovec count_vec = {
            .iov_base = child_buf,
            .iov_len = sizeof child_buf,
        };

        struct msghdr child_msg = {
            .msg_control = buf,
            .msg_controllen = sizeof buf,
            .msg_iov = &count_vec,
            .msg_iovlen = 1,
        };

        if (recvmsg(sd[1], &child_msg, 0) < 0)
            perror("child recv msg 1");
        cmsg = CMSG_FIRSTHDR(&child_msg);
        if (cmsg == NULL) {
            printf("no cmsg rcvd in child");
            return;
        }
        recv_fd = *(int*)CMSG_DATA(cmsg);
        if (recv_fd < 0) {
            printf("could not get recv_fd from socket");
            return;
        }
        printf("child share fd is %d, it is from master process\n", recv_fd);
        ion_open();
        ptr = mmap(NULL, len, prot, map_flags, recv_fd, 0);
        if (ptr == MAP_FAILED) {
            return;
        }
        printf("[child] recev first msg: [%10s], it should be `master`\n", ptr);
        strcpy(ptr, "child");
        printf("child sending msg 2 to parent\n");
        sendmsg(sd[1], &child_msg, 0);
    }
}



void ion_import_test()

{
    ion_user_handle_t handle;
    int sd[2];
    int num_fd = 1;
    struct iovec count_vec = {
        .iov_base = &num_fd,
        .iov_len = sizeof num_fd,
    };
    printf("ion_import_test start\n");
    char buf[CMSG_SPACE(sizeof(int))];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sd);
    if (fork()) {
        struct msghdr msg = {
            .msg_control = buf,
            .msg_controllen = sizeof buf,
            .msg_iov = &count_vec,
            .msg_iovlen = 1,
        };

        struct cmsghdr *cmsg;
        int fd, share_fd, ret;
        char *ptr;
        /* parent */
        if(_ion_alloc_test(&fd, &handle))
            return;
        ret = ion_share(fd, handle, &share_fd);
        if (ret)
            printf("share failed %s\n", strerror(errno));
        ptr = mmap(NULL, len, prot, map_flags, share_fd, 0);
        if (ptr == MAP_FAILED) {
            return;
        }
        strcpy(ptr, "master");
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        *(int *)CMSG_DATA(cmsg) = share_fd;
        /* send the fd */
        printf("[master] prepare sending msg to child, content [%10s] should be `master`\n", ptr);

        sendmsg(sd[0], &msg, 0);
        if (recvmsg(sd[0], &msg, 0) < 0)
            perror("[master] recv msg from child");
        printf("[master] recv msg from child, content is [%10s] , it should be `child`\n", ptr);

        /* send ping */
        sendmsg(sd[0], &msg, 0);
        printf("master->master? [%10s]\n", ptr);
        if (recvmsg(sd[0], &msg, 0) < 0)
            perror("master recv 1");
    } else {
        //struct msghdr msg;
        struct cmsghdr *cmsg;
        char* ptr;
        int  recv_fd;
        char* child_buf[100];
        /* child */
        struct iovec count_vec = {
            .iov_base = child_buf,
            .iov_len = sizeof child_buf,
        };

        struct msghdr child_msg = {
            .msg_control = buf,
            .msg_controllen = sizeof buf,
            .msg_iov = &count_vec,
            .msg_iovlen = 1,
        };

        if (recvmsg(sd[1], &child_msg, 0) < 0)
            perror("child recv msg 1");
        cmsg = CMSG_FIRSTHDR(&child_msg);
        if (cmsg == NULL) {
            printf("no cmsg rcvd in child");
            return;
        }
        recv_fd = *(int*)CMSG_DATA(cmsg);
        if (recv_fd < 0) {
            printf("could not get recv_fd from socket");
            return;
        }
        printf("child share fd is %d, it is from master process\n", recv_fd);
        int fd1 = ion_open();

        ion_phys_addr_t phys_addr;
        ion_cpu_addr_t cpu_addr;
        size_t size;
        ion_user_handle_t child_handle;
        
        int ret = ion_import(fd1, recv_fd, &child_handle);
        if (ret)
        {
            printf("%s ion_import failed: %s %d\n", __func__, strerror(ret), child_handle);
            return;
        }
        
        ret = ion_get_info(fd1, child_handle,&phys_addr,&cpu_addr,&size);
        if (ret) {
            printf("%s ion_get_info failed: %s %d\n", __func__, strerror(ret), handle);
            return;
        }
        printf("%s ion_get_info size is : %d\n", __func__,(int)size);
        

        ptr = mmap(NULL, len, prot, map_flags, recv_fd, 0);
        if (ptr == MAP_FAILED) {
            return;
        }
        printf("[child] recev first msg: [%10s], it should be `master`\n", ptr);
        strcpy(ptr, "child");
        printf("child sending msg 2 to parent\n");
        sendmsg(sd[1], &child_msg, 0);
    }
}

int main(int argc, char* argv[]) {


    ion_alloc_many_buf_test();
    ion_alloc_test();

    ion_map_test();
    //ion_share_test();
    ion_get_info_get();
    ion_import_test();
    return 0;
}
