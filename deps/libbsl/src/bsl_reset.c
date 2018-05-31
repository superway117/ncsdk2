
//#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <stdarg.h>

#include "getopt.h"

#include "hddl-i2c.h"
#include "i2cbusses.h"


#define HDDL_SMBUS_NAME ("SMBus I801 adapter at f040")
#define HDDL_RESET_REG (0x1F)
#define HDDL_RESET_ADDRESS (0x1)
#define HDDL_RESET_VALUE (0xFF)

static void help(void)
{

  fprintf(stderr,
      "Usage: bsl_reset -d [io|mcu] -i [device_id]\n"
      );
}
#ifndef WIN32
static void print_i2c_busses(void)
{
  struct i2c_adap *adapters;
  int count;

  adapters = i2c_busses_gather();
  if (adapters == NULL) {
    fprintf(stderr, "Error: Out of memory!\n");
    return;
  }
  printf("--------------all i2c/smbus devices------------\n");
  for (count = 0; adapters[count].name; count++) {
    printf("i2c-%d\t%-10s\t%-32s\t%s\n",
      adapters[count].nr, adapters[count].funcs,
      adapters[count].name, adapters[count].algo);
  }

  free_adapters(adapters);
}
#endif

int main(int argc, char *argv[])
{
#ifndef WIN32
    print_i2c_busses();
#endif
    help();
    int opt;
    int deviceid = 0xFF;

    while(1)
    {
        opt = getopt(argc, argv, "i:d:");  
        if (opt < 0)  
        {  
            break;  
        }
        switch(opt)
        {
            case 'd':
         
            if(strcmp("mcu",optarg)==0)
                hddl_set_i2c_device(I2C_MCU);
            else if(strcmp("io",optarg)==0)
                hddl_set_i2c_device(I2C_IOEXPANDER);
            break;
            case 'i':
         
            deviceid = atoi(optarg);
            break;  
        }
    }

    int res=0;

    if (deviceid != 0xFF)
    {
        printf("Try to reset device with DeviceID: %d\n",deviceid);
        res = hddl_reset(deviceid);
        if(res)
            printf("Reset devcie failed with result %d\n",res);
    }
    else
    {
        res = hddl_reset_all();
        if(res)
            printf("Reset all devcie on address failed with result %d\n",res);
        else
            printf("Reset all devcie succ\n");
    }
    exit(0);
}
