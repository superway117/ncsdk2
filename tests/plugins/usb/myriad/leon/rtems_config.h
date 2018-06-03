///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief    RTEMS configuration
///

#ifndef _LEON_RTEMS_CONFIG_H_
#define _LEON_RTEMS_CONFIG_H_

#include "fatalExtension.h"
#include "OsDrvInit.h"

#if defined(__RTEMS__)

#if !defined (__CONFIG__)
#define __CONFIG__

/* ask the system to generate a configuration table */
#define CONFIGURE_INIT

#ifndef RTEMS_POSIX_API
#define RTEMS_POSIX_API
#endif

#define CONFIGURE_MICROSECONDS_PER_TICK         1000    /* 1 millisecond */

#define CONFIGURE_TICKS_PER_TIMESLICE           50      /* 50 milliseconds */

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE      (4096)
#define CONFIGURE_MAXIMUM_TASKS                 8
#define CONFIGURE_MAXIMUM_POSIX_THREADS         8
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES         8
#define CONFIGURE_MAXIMUM_POSIX_KEYS            8
#define CONFIGURE_MAXIMUM_TIMERS                4
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES        1
#define CONFIGURE_MAXIMUM_SEMAPHORES            16
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES      8
#define CONFIGURE_MAXIMUM_DRIVERS               16
#define CONFIGURE_MAXIMUM_DEVICES               16
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 10
#define CONFIGURE_POSIX_INIT_TASKS_TABLE

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS    1
#define CONFIGURE_INITIAL_EXTENSIONS         { .fatal = Fatal_extension }

#define CONFIGURE_APPLICATION_EXTRA_DRIVERS OS_DRV_INIT_TABLE_ENTRY

void *POSIX_Init (void *args);
#include <rtems/confdefs.h>

#endif // __CONFIG__

#endif // __RTEMS__

// 3:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 4:  Functions (non-inline)
// ----------------------------------------------------------------------------

#endif /* _LEON_RTEMS_CONFIG_H_ */

