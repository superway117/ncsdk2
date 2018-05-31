
#ifndef __HDDL_BSL_PRIV_H__
#define __HDDL_BSL_PRIV_H__

#define I2C_MAX_I2C_ADDR_NUM 						 (4)
#define I2C_DEVICE_MAX_NAME_LEN 				 (20)
#define I2C_INVALID_I2C_ADDR 						 (0xFFFFFFFF)

typedef int  (*device_init_t)();
typedef int  (*device_reset_t)(int);
typedef int  (*device_reset_all_t)();
typedef int  (*device_add_t)(int);
typedef int  (*device_remove_t)(int);
typedef int  (*address_is_valid_t)(int);
typedef void (*set_address_scale_t)(int, int);
typedef void (*get_address_scale_t)(int*, int*);

typedef struct {
    device_init_t       device_init;
    device_reset_t      device_reset;
    device_reset_all_t  device_reset_all;
    device_add_t        device_add;
    device_remove_t     device_remove;
    address_is_valid_t  address_is_valid;
    set_address_scale_t set_address_scale;
    get_address_scale_t get_address_scale;
}HddlController_t;

int i2c_write_byte(int i2c_addr,int reg,int value);
int i2c_read_byte(int i2c_addr,int reg,int* value);
int i2c_fetch_address_by_scan(int start_addr, 
                              int end_addr, 
                              int* dev_addr);
int bsl_fetch_address_from_config(I2CDeviceType* type, 
                                  int* dev_addr);

void mcu_slave_init(HddlController_t* ctrl);
void ioexpander_init(HddlController_t* ctrl);

#endif 

