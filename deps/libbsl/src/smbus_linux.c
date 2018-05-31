

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"
#include "i2c-dev.h"
#include "i2cbusses.h"
 
#define MODE_AUTO   0
#define MODE_QUICK  1
#define MODE_READ   2
#define MODE_FUNC   3

#define HDDL_SMBUS_NAME                                 ("SMBus I801 adapter at")


static int _check_funcs(int file, int size, int pec)
{
    unsigned long funcs;

    if (ioctl(file, FUNCTIONS, &funcs) < 0) {
        fprintf(stderr, "Error: Could not get the adapter "
            "functionality matrix: %s\n", strerror(errno));
        return -1;
    }

    switch (size) {

    case I2C_SMBUS_BYTE_DATA:
        if (!(funcs & FUNCTION_SMBUS_WRITE_BYTE_DATA)) {
            fprintf(stderr, MISSING_FUNC_FMT, "SMBus write byte");
            return -1;
        }
        break;
    }

    if (pec
     && !(funcs & (FUNCTION_SMBUS_PEC | FUNCTION_I2C))) {
        fprintf(stderr, "Warning: Adapter does "
            "not seem to support PEC\n");
    }

    return 0;
}

static int i2c_file_open()
{
    int i2cbus, file;
 

    i2cbus = i2c_bus_lookup(HDDL_SMBUS_NAME);
    if (i2cbus < 0)
    {
        return -2;
    }
    file = i2c_dev_open(i2cbus, /*filename, sizeof(filename), */0);
    if (file < 0) {
        return -3;
    }
    return file;
}


static int i2c_check_funcs(int file,int i2c_addr)
{
     
    unsigned long funcs;
     
    if(_check_funcs(file, I2C_SMBUS_BYTE_DATA, 0)
        || slave_addr_set(file, i2c_addr, 1))
    {
        return -3;
    }

    if (ioctl(file, FUNCTIONS, &funcs) < 0)
    {
        return -4;
    }

    return 0;
}

static int i2c_file_close(int file)
{
    close(file);
    return 0;
}

int i2c_fetch_address_by_scan(int start_addr, 
                              int end_addr, 
                              int* dev_addr)
{
    int i, j;
    int res;
    int find_index = 0;
    int mode = MODE_AUTO;
    int file = i2c_file_open();
    
    for (i = 0; i < 128; i += 16) 
    {
        for(j = 0; j < 16; j++) 
        {
            /* Skip unwanted addresses */
            if (i+j < start_addr) {
                continue;
            }
            if (i+j > end_addr) {
                break;
            }

            /* Set slave address */
            if (ioctl(file, I2C_SLAVE, i+j) < 0) 
            {
                if (errno == EBUSY) 
                {
                    continue;
                } else {
                    i2c_file_close(file);
                    return -1;
                }
            }

            /* Probe this address */
            switch (mode) {
            case MODE_QUICK:
                /* This is known to corrupt the Atmel AT24RF08
                   EEPROM */
                res = __i2c_smbus_write_quick(file,
                      I2C_SMBUS_WRITE);
                break;
            case MODE_READ:
                /* This is known to lock SMBus on various
                   write-only chips (mainly clock chips) */
                res = __i2c_smbus_read_byte(file);
                break;
            default:
                if ((i+j >= 0x30 && i+j <= 0x37)
                 || (i+j >= 0x50 && i+j <= 0x5F))
                    res = __i2c_smbus_read_byte(file);
                else
                    res = __i2c_smbus_write_quick(file,
                          I2C_SMBUS_WRITE);
            }

            if (res < 0)
                continue;
            else
            {
                if (find_index < I2C_MAX_I2C_ADDR_NUM)
                {
                    dev_addr[find_index] = i + j;
                    find_index++;
                }
                if (find_index >= I2C_MAX_I2C_ADDR_NUM)
                    break;
            }
        }
    }
    
    i2c_file_close(file);
    return find_index;
}
 
int i2c_write_byte(int i2c_addr,int reg,int value)
{
   
    int file  = i2c_file_open();
    int res = -1;
    res =  i2c_check_funcs(file,i2c_addr);
    if(res)
    {
        close(file);
        return res;
    }
    res = __i2c_smbus_write_byte_data(file, reg, value);
 
    close(file);
     
    return res;

}


int i2c_read_byte(int i2c_addr,int reg,int* value)
{
   
    int file  = i2c_file_open();
    int res = -1;
  
    res =  i2c_check_funcs(file,i2c_addr);
    if(res)
    {
        close(file);
        return res;
    }
    res = __i2c_smbus_read_byte_data(file, reg);
    if(res >= 0)
    {
        *value = res;
        res = 0;
    } 
    close(file);
    return res;

}



