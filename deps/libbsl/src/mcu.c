
#include <stdio.h>

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"

#ifdef DEBUG_MCU
#define DBG_MCU printf
#else
#define DBG_MCU(...) {}
#endif

static int m_mcu_address = I2C_INVALID_I2C_ADDR; // Only support one
static int m_mcu_start_addr = 0x18;
static int m_mcu_end_addr   = 0x1F;

static int m_mcu_init()
{
	return 0;
}

static void m_mcu_parse_device_id(int device_id,int* real_device_id)
{
	char data = *((char*)&device_id);
	*real_device_id = (int)(data & 0x07);
}

//only support one mcu
static int m_mcu_reset(int device_id)
{
 	int real_device_id=0;
 	m_mcu_parse_device_id(device_id,&real_device_id);
 	int reset_value = 1<<real_device_id;
    DBG_MCU("_mcu_reset reset_value value is %d\n",reset_value);
 	if(m_mcu_address == I2C_INVALID_I2C_ADDR)
 		return -1;
 	return i2c_write_byte(m_mcu_address, 0x01, reset_value);
}

static int m_mcu_reset_all()
{
    return i2c_write_byte(m_mcu_address, 0x01, 0xFF);
}

static void m_mcu_get_address_scale(int* start, int* end)
{
    *start = m_mcu_start_addr;
    *end   = m_mcu_end_addr;
}

static void m_mcu_set_address_scale(int start, int end)
{
    if (start > end)
    {
        return;
    }
    m_mcu_start_addr = start;
    m_mcu_end_addr   = end;
}

static int m_mcu_address_valid(int address)
{
    if (address > m_mcu_end_addr || address < m_mcu_start_addr)
    {
        return 0;
    }
    return 1;
}

static int m_mcu_set_address(int slave_addr)
{
    if (m_mcu_address_valid (slave_addr))
    {
        m_mcu_address = slave_addr;
        return 0;
    }
    return -1;
}

void mcu_slave_init(HddlController_t* ctrl)
{
    ctrl->device_init           = m_mcu_init;
    ctrl->device_reset          = m_mcu_reset;
    ctrl->device_reset_all      = m_mcu_reset_all;
    ctrl->device_add            = m_mcu_set_address;
    ctrl->device_remove         = NULL;
    ctrl->address_is_valid      = m_mcu_address_valid;
    ctrl->set_address_scale     = m_mcu_set_address_scale;
    ctrl->get_address_scale     = m_mcu_get_address_scale;
}


