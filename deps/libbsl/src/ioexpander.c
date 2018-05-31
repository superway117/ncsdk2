
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep Sleep
#endif

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"

static int m_ioexpander_address_list[I2C_MAX_I2C_ADDR_NUM];
static int m_ioexpander_start_addr = 0x20;
static int m_ioexpander_end_addr   = 0x27;
static unsigned char m_ioexpander_count = 0;

#ifdef DEBUG_IOEXPANDER
#define DBG_IO printf
#else
#define DBG_IO(...) {}
#endif

static int m_ioexpander_address_valid(int address)
{
    if (address > m_ioexpander_end_addr || address < m_ioexpander_start_addr)
    {
        return 0;
    }
    return 1;
}

//config all pin as output
int m_ioexpander_init()
{
    int ret = 0;

	//# Configure PORT0 as output
	//sudo i2cset -y 5 0x20 0x06 0x00 b
	//registers 6 and 7) shown in Table 7 configure the directions
	//of the I/O pins. If a bit in
	//this register is set to 1, the corresponding port pin is 
	//enabled as an input with a high-impedance output driver. If
	//a bit in this register is cleared to 0, the corresponding 
	//port pin is enabled as an output.

	static int initialized = 0;
	if(initialized==0)
	{
		int i=0;
		 
		for(i = 0; i < m_ioexpander_count; i++)
		{
			int i2c_addr = m_ioexpander_address_list[i];
            DBG_IO("_ioexpander_pre i2c_addr is %x\n",i2c_addr);
			ret = i2c_write_byte(i2c_addr, 0x06, 0x00);
		}
		initialized = 1;
	}
	return ret;
}

// high 3 bits is baord id
// low 5 bits is real device id
static void m_ioexpander_parse_device_id(int device_id,int* i2c_addr,int* real_device_id)
{
	char data = *((char*)&device_id);
	int board_id = (int)((data & 0xE0) >> 5);
	*real_device_id = (int)(data & 0x1F);
	*i2c_addr = 0x20 + board_id;
}

static int m_ioexpander_reset(int device_id)
{
	int i2c_addr = I2C_INVALID_I2C_ADDR;
	int value = 0;
	int real_device_id=0;
	m_ioexpander_parse_device_id(device_id, &i2c_addr, &real_device_id);

 	int res =  i2c_read_byte(i2c_addr, 0x02, &value);
 	if(res)
 	{
 		return res;
 	}
    DBG_IO("_ioexpander_reset origin value is %d\n",value);
 
 	int reset_value = 1<<real_device_id;
 	
 	int down_value = value & (~reset_value);
    DBG_IO("_ioexpander_reset down_value is %d\n",down_value);
 	res =  i2c_write_byte(i2c_addr, 0x02, down_value);
 	sleep(1);
 	int up_value = value | reset_value;
    DBG_IO("_ioexpander_reset up_value is %d\n",up_value);
 	res =  i2c_write_byte(i2c_addr, 0x02, up_value);
 	return res;
}

static int m_ioexpander_reset_all()
{
	//flip-flop controlling the output selection
	int i2c_addr = I2C_INVALID_I2C_ADDR;
	int i=0;
	int ret = 0;
			 
	for (i = 0; i < m_ioexpander_count; i++)
	{
		i2c_addr = m_ioexpander_address_list[i];
        DBG_IO("_ioexpander_reset_all i2c_addr is %x\n",i2c_addr);
		if(i2c_addr == I2C_INVALID_I2C_ADDR)
			break;
		ret = i2c_write_byte(i2c_addr, 0x02, 0x00);
		if(ret)
		{
			return ret;
		}
		sleep(1);
		ret = i2c_write_byte(i2c_addr, 0x02, 0xFF);
	}
	return ret;
}

static int m_ioexpander_add_slave_address(int slave_addr)
{
    if (m_ioexpander_count >= I2C_MAX_I2C_ADDR_NUM)
    {
        return -1;
    }
    if (!m_ioexpander_address_valid (slave_addr))
    {
        return -1;
    }
    m_ioexpander_address_list[m_ioexpander_count] = slave_addr;
    m_ioexpander_count++;
    return 0;
}

static int m_ioexpander_remove_slave_address(int slave_addr)
{
    int i;
    if (m_ioexpander_count == 0)
    {
        return -1;
    }
    for (i = 0; i < m_ioexpander_count; i++)
    {
        if (m_ioexpander_address_list[i] == slave_addr)
        {
            // Found device and delete it
            m_ioexpander_count--;
            m_ioexpander_address_list[i] = m_ioexpander_address_list[m_ioexpander_count];
            return 0;
        }
    }
    return -1;
}

static void m_ioexpander_get_address_scale(int* start, int* end)
{
    *start = m_ioexpander_start_addr;
    *end   = m_ioexpander_end_addr;
}

static void m_ioexpander_set_address_scale(int start, int end)
{
    if (start > end)
    {
        return;
    }
    m_ioexpander_start_addr = start;
    m_ioexpander_end_addr   = end;
}

void ioexpander_init(HddlController_t* ctrl)
{
    ctrl->device_init           = m_ioexpander_init;
    ctrl->device_reset          = m_ioexpander_reset;
    ctrl->device_reset_all      = m_ioexpander_reset_all;
    ctrl->device_add            = m_ioexpander_add_slave_address;
    ctrl->device_remove         = m_ioexpander_remove_slave_address;
    ctrl->address_is_valid      = m_ioexpander_address_valid;
    ctrl->set_address_scale     = m_ioexpander_set_address_scale;
    ctrl->get_address_scale     = m_ioexpander_get_address_scale;
}

