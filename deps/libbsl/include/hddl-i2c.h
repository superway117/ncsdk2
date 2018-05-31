
#ifndef LIB_HDDL_I2C_SMBUS_H
#define LIB_HDDL_I2C_SMBUS_H

typedef enum {
  I2C_IOEXPANDER = 0,
  I2C_MCU,
  I2C_DEVICE_TYPE_MAX
} I2CDeviceType;

int hddl_reset(int device_addr);

int hddl_reset_all();
int hddl_bsl_init();
I2CDeviceType hddl_get_i2c_device();

void hddl_set_i2c_device(I2CDeviceType i2c_device);

#endif /* LIB_HDDL_I2C_SMBUS_H */
