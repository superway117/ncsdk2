#ifndef _MYD_VSC_H_
#define _MYD_VSC_H_

#include <linux/ioctl.h>
#define IOC_MYD_TYPE	't'
#define IOC_MYD_MIN_NR	30
#define IOC_MYD_WRITE	_IOWR(IOC_MYD_TYPE, 30, struct __myd_ionrw_data)
#define IOC_MYD_READ	_IOWR(IOC_MYD_TYPE, 31, struct __myd_ionrw_data)
#define IOC_MYD_MAX_NR	31

typedef int ion_user_handle_t;

typedef struct __myd_ionrw_data {
  ion_user_handle_t handle;
  size_t len;
  unsigned long phys_addr;
  unsigned long cpu_addr;
}myd_write_data_t,myd_read_data_t;


#endif

