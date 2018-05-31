
#include <stdio.h>

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"


HddlController_t m_hddl_controller[I2C_DEVICE_TYPE_MAX];

static I2CDeviceType m_i2c_device = I2C_MCU;

I2CDeviceType hddl_get_i2c_device()
{
	return m_i2c_device;
}

void hddl_set_i2c_device(I2CDeviceType i2c_device)
{
	if (m_i2c_device != i2c_device && 
        i2c_device < I2C_DEVICE_TYPE_MAX)
	{
		m_i2c_device = i2c_device;
	}
}

int hddl_bsl_init()
{
    int i;
    I2CDeviceType dev_type = I2C_DEVICE_TYPE_MAX;
    int address_list[I2C_MAX_I2C_ADDR_NUM];
    int device_cnt;
    
    mcu_slave_init(&m_hddl_controller[I2C_MCU]);
    ioexpander_init(&m_hddl_controller[I2C_IOEXPANDER]);
    
    device_cnt = bsl_fetch_address_from_config(&dev_type, address_list);
    if (device_cnt > 0 && 
        dev_type < I2C_DEVICE_TYPE_MAX)
    {
        goto FILL_PARAMETER;
    }
    for (i = 0; i < I2C_DEVICE_TYPE_MAX; i++)
    {
        int start_addr;
        int end_addr;
        m_hddl_controller[i].get_address_scale(&start_addr, 
                                                &end_addr);
        device_cnt = i2c_fetch_address_by_scan (start_addr, 
                                                end_addr, 
                                                address_list);
        if (device_cnt > 0)
        {
            dev_type = i;
            goto FILL_PARAMETER;
        }
    }
    return -1;
FILL_PARAMETER:
    printf("Device type: %d, device cnt: %d\r\n", dev_type, device_cnt);
    hddl_set_i2c_device(dev_type);
    for (i = 0; i < device_cnt; i++)
    {
        m_hddl_controller[dev_type].device_add(address_list[i]);
    }
    return 0;
}

#ifndef WIN32
__attribute__((constructor))
#endif
void libbsl_init()
{
    hddl_bsl_init();
}


#ifdef WIN32
typedef void (__cdecl *PF)();
#pragma section(".CRT$XCG", read)
__declspec(allocate(".CRT$XCG")) PF lib_construct[1] = { 
    libbsl_init 
};
#endif

int hddl_reset_all()
{
 	int res = 0;
	if(m_hddl_controller[m_i2c_device].device_init)
	{
		res = m_hddl_controller[m_i2c_device].device_init();
	}
	if(res)
	{
		return res;
	}

	if(m_hddl_controller[m_i2c_device].device_reset_all)
	{
		return m_hddl_controller[m_i2c_device].device_reset_all();
	}
	return -1;
}
 
int hddl_reset(int device_addr)
{
	int res = 0;
	if(m_hddl_controller[m_i2c_device].device_init)
	{
		res = m_hddl_controller[m_i2c_device].device_init();
	}
    
	if(res)
	{
		return res;
	}

	if(m_hddl_controller[m_i2c_device].device_reset)
	{
		return m_hddl_controller[m_i2c_device].device_reset(device_addr);
	}
	return -1;
}

