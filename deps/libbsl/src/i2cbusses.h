
#ifndef _I2CBUSSES_H
#define _I2CBUSSES_H

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct i2c_adap *i2c_busses_gather(void);
void free_adapters(struct i2c_adap *adapters);

int i2c_bus_lookup(const char *i2cbus_arg);
int i2c_address_parse(const char *address_arg);
int i2c_dev_open(int i2cbus, /*char *filename, size_t size, */int quiet);
int slave_addr_set(int file, int address, int force);

#define MISSING_FUNC_FMT	"Error: Adapter does not have %s capability\n"

#endif
