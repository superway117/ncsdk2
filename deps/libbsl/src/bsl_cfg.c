
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <json-c/json.h>

#ifndef WIN32
#include <linux/limits.h>
#else
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#endif

#include "hddl-i2c.h"
#include "hddl_bsl_priv.h"


#define MAX_PATH_LENGTH    PATH_MAX

const static char m_cfg_device_type_str[I2C_DEVICE_TYPE_MAX][I2C_DEVICE_MAX_NAME_LEN] = {
    "ioexpander",
    "mcu"
};

I2CDeviceType m_get_device_type_by_name(const char* dev_name)
{
	int i;
	int len = sizeof(m_cfg_device_type_str)/sizeof(m_cfg_device_type_str[0]);
	assert(dev_name);
	for (i = 0; i < len; i++)
	{
		if (0 == strcmp(dev_name, m_cfg_device_type_str[i]))
		{
			return i;
		}
	}
	return I2C_DEVICE_TYPE_MAX;
}

static int _load_cfg(const char* file_path, 
                     I2CDeviceType* type, 
                     int* dev_addr)
{
    FILE *fp;
    char *buf;

    int find_num = 0;
    fp = fopen(file_path, "rb");
    if(fp == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);
    unsigned int length = ftell(fp);
    rewind(fp);

    if(!(buf = malloc(length)))
    {
        printf("malloc failed\n");
        fclose(fp);
        return -1;
    }
    if(fread(buf, 1, length, fp) != length)
    {
        printf("read failed\n");
        fclose(fp);
        free(buf);
        return -1;
    }

    fclose(fp);

    struct json_object*  obj = json_tokener_parse(buf);
    if(obj == NULL)
    {
        printf("json object is null\n");
        free(buf);
        return -1;
    }


    struct json_object* jdev = json_object_object_get(obj,"active");
    assert(jdev);
    I2CDeviceType devType = m_get_device_type_by_name(json_object_get_string(jdev));
    assert(devType < I2C_DEVICE_TYPE_MAX);
    *type = devType;

    //strcpy((char*)dev->name,json_object_get_string(jdev));
    struct json_object* jaddress = json_object_object_get(obj,"i2c_addr");
    assert(jaddress);
    size_t addr_len = json_object_array_length(jaddress);
    if(addr_len==0)
    {
        printf("i2c address is not set in json\n");
        free(buf);
        return -1;
    }
    int j = 0;
    if (addr_len > I2C_MAX_I2C_ADDR_NUM)
    {
        addr_len = I2C_MAX_I2C_ADDR_NUM;
    }

    for (j = 0; j < addr_len; j++)
    {
        struct json_object* addr_obj = json_object_array_get_idx(jaddress,j);
        int value = json_object_get_int(addr_obj);
        //printf("%d\n",value);
        //printf("dev->start_addr = %d\n",dev->start_addr);
        //printf("dev->end_addr = %d\n",dev->end_addr);
        find_num++;
        dev_addr[j] = value;
    }
    
    free(buf);

    printf("_load_cfg find %d;dev is %s\n", find_num, m_cfg_device_type_str[devType]);

    return find_num;
}

int bsl_fetch_address_from_config(I2CDeviceType* type, 
                                  int* dev_addr)
{
    char* envValue = getenv("HDDL_INSTALL_DIR");
    if(!envValue) {
	    return 0;
    }
    char localStr[MAX_PATH_LENGTH];
    snprintf(localStr, sizeof(localStr), "%s/config/bsl.json", envValue);
    return _load_cfg (localStr, type, dev_addr);
}

