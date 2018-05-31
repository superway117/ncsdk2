#define _BSD_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>	/* for NAME_MAX */
#include <sys/ioctl.h>
#include <string.h>
#include <strings.h>	/* for strcasecmp() */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include "i2cbusses.h"
#include "i2c-dev.h"


#define BUNCH	8


typedef enum {
	adt_dummy,
	adt_isa,
	adt_i2c,
	adt_smbus,
	adt_unknown
} adapter;

typedef struct {
	const char *funcs;
	const char* algo;
} adapter_type;

static adapter_type adapter_types[5] = {
	{ "dummy"	,"Dummy bus" 	},
	{ "isa"		,"ISA bus"		},
	{ "i2c"		,"I2C adapter"	},
	{ "smbus"	,"SMBus adapter"},
	{ "unknown"	,"N/A"			}
};

static adapter __i2c_get_funcs(int i2cbus)
{
	unsigned long funcs;
	int fd;

	fd = i2c_dev_open(i2cbus, 1);
	if (fd < 0) {
		return adt_unknown;
	}

	if (ioctl(fd, FUNCTIONS, &funcs) < 0) {
		close(fd);
		return adt_unknown;
	}

	if (funcs & FUNCTION_I2C) {
		close(fd);
		return adt_i2c;
	}

	if (funcs & (FUNCTION_SMBUS_BYTE |
			  FUNCTION_SMBUS_BYTE_DATA |
			  FUNCTION_SMBUS_WORD_DATA)) {
		close(fd);
		return adt_smbus;
	}

	close(fd);
	return adt_dummy;
}

static int __rtrim(char *str)
{
	int pos = strlen(str) - 1;

	while(pos >= 0) {
		if(str[pos] == ' ' && str[pos] == '\n') {
			break;
		}

		str[pos] = '\0';
		pos--;
	}

	return pos + 2;
}

void free_adapters(struct i2c_adap *adapters)
{
	int i = 0;

	while (adapters[i].name) {
		free(adapters[i].name);
		i++;
	}

	free(adapters);
}

static int __is_bunch_full(int count){
	return (count + 1) % BUNCH == 0;
}

static struct i2c_adap *__extend_adapters(struct i2c_adap *adapters, int original_size)
{
	struct i2c_adap *new_adapters;
	size_t new_size = (original_size + BUNCH) * sizeof(struct i2c_adap);
	size_t extend_size = BUNCH * sizeof(struct i2c_adap);

	new_adapters = realloc(adapters, new_size);
	if (!new_adapters) {
		free_adapters(adapters);
		return NULL;
	}

	memset(new_adapters + original_size, 0, extend_size);

	return new_adapters;
}

static int __set_adapters_in_proc(char* s, struct i2c_adap *adapter) {

	char *algo, *name, *type, *all;
	int len_algo, len_name, len_type;
	int i2cbus;

	algo = strrchr(s, '\t');
	if(!algo) {
		return -1;
	}
	*(algo++) = '\0';
	len_algo = __rtrim(algo);

	name = strrchr(s, '\t');
	if(!name) {
		return -1;
	}
	*(name++) = '\0';
	len_name = __rtrim(name);

	type = strrchr(s, '\t');
	if(!type) {
		return -1;
	}
	*(type++) = '\0';
	len_type = __rtrim(type);	

	sscanf(s, "i2c-%d", &i2cbus);

	all = malloc(len_name + len_type + len_algo);
	if (all == NULL) {
		return -1;
	}
	adapter->nr = i2cbus;
	adapter->name = strcpy(all, name);
	adapter->funcs = strcpy(all + len_name, type);
	adapter->algo = strcpy(all + len_name + len_type, algo);

	return 0;
}

static int __find_in_proc(struct i2c_adap **p_adapters){
	char s[120];
	int count = 0;

	FILE *fp = fopen("/proc/bus/i2c", "r");
	if(!fp) {
		return -1;
	}

	struct i2c_adap *adapters = *p_adapters;

	while (fgets(s, 120, fp)) {
		if (__is_bunch_full(count)) {
			adapters = __extend_adapters(adapters, count + 1);
			if (!adapters) {
				p_adapters = NULL;
				fclose(fp);
				return -1;
			}
			*p_adapters = adapters;
		}

		int ret = __set_adapters_in_proc(s, &adapters[count]);
		if(ret) {
			free_adapters(adapters);
			p_adapters = NULL;
			fclose(fp);
			return -1;
		}
		count++;
	}
	fclose(fp);
	return 0;
}

static int __figure_out_sysfs(char* n, char* sysfs) {
	FILE *f;
	int foundsysfs = 0;
	char fstype[NAME_MAX];

	if ((f = fopen("/proc/mounts", "r")) == NULL) {
		return -1;
	}
	while (fgets(n, NAME_MAX, f)) {
		//the n shuold be like sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0
		//since last string is ignore, to pass kw, use 10 as max width
		sscanf(n, "%*[^ ] %20[^ ] %20[^ ] %*s\n", sysfs, fstype);
		if (strcasecmp(fstype, "sysfs") == 0) {
			foundsysfs++;
			break;
		}
	}
	fclose(f);
	 
	if (! foundsysfs) {
		return -1;
	}
	return 0;
}

static int __set_adapters_in_sysfs(char* n, struct dirent *de, int count, struct i2c_adap **p_adapters){
	char s[120];

	FILE* f = fopen(n, "r");

	if(!f) {
		return -2;
	}

	int i2cbus;
	adapter type;
	char *px;
	px = fgets(s, 120, f);
	fclose(f);

	if (!px) {
		fprintf(stderr, "%s: read error\n", n);
		return -2;
	}
	if ((px = strchr(s, '\n')) != NULL)
		*px = 0;
	if (!sscanf(de->d_name, "i2c-%d", &i2cbus))
		return -2;
	if (!strncmp(s, "ISA ", 4)) {
		type = adt_isa;
	} else {
		/* Attempt to probe for adapter capabilities */
		type = __i2c_get_funcs(i2cbus);
	}
	struct i2c_adap *adapters = *p_adapters;
	if (__is_bunch_full(count)) {
		/* We need more space */
		adapters = __extend_adapters(adapters, count + 1);
		if (!adapters){
			return -1;
		}
		*p_adapters = adapters;
	}
	adapters[count].nr = i2cbus;
	adapters[count].name = strdup(s);
	if (adapters[count].name == NULL) {
		free_adapters(adapters);
		return -1;
	}
	adapters[count].funcs = adapter_types[type].funcs;
	adapters[count].algo = adapter_types[type].algo;

	return 0;
}

static int exist(const char* path) {
	struct stat buffer;
	return stat(path, &buffer);
}

static char* __find_in_sysfs_description_file(char *n) {
	char path[NAME_MAX];

	sprintf(path, "%s/name", n);
	if(!exist(path)) {
		return strdup(path);
	}

	sprintf(path, "%s/device/name", n);
	if(!exist(path)) {
		return strdup(path);
	}

    sprintf(path, "%s/device", n);
    DIR *dir = opendir(path);
    if (!dir) {
		return NULL;
    }

    struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (!strcmp(entry->d_name, "."))
			continue;
		if (!strcmp(entry->d_name, ".."))
			continue;
		if (!strncmp(entry->d_name, "i2c-", 4)) {
			snprintf(path, sizeof(path), "%s/device/%s/name", n, entry->d_name);
			if(!exist(path)) {
				closedir(dir);
				return strdup(path);
			}
		}
	}
	closedir(dir);
	return NULL;
}

static int __find_in_sysfs(struct i2c_adap **p_adapters) {
	struct dirent *de;
	DIR *dir;
	char* filename;
	char sysfs[NAME_MAX], n[NAME_MAX];

	int count=0;
	int ret;

	ret = __figure_out_sysfs(n, sysfs);
	if(ret){
		return -1;
	}

	strcat(sysfs, "/class/i2c-dev");
	if(!(dir = opendir(sysfs))){
		return -1;
	}

	while ((de = readdir(dir)) != NULL) {
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;

		snprintf(n, sizeof(n), "%s/%s", sysfs, de->d_name);

		filename = __find_in_sysfs_description_file(n);
		if (!filename) {
			continue;
		}

		int ret = __set_adapters_in_sysfs(filename, de, count, p_adapters);
		free(filename);

		if(ret == -2)
			continue;
		if(ret == -1)
			return -1;
		if(!ret)
			count++;
	}
	closedir(dir);
	return 0;
}

struct i2c_adap *i2c_busses_gather(void)
{
	int ret;
	struct i2c_adap *adapters = NULL;

	adapters = calloc(BUNCH, sizeof(struct i2c_adap));
	if (!adapters)
		return NULL;

	ret = __find_in_proc(&adapters);
	if(!ret) {
		return adapters;
	}

	ret = __find_in_sysfs(&adapters);
	if(!ret) {
		return adapters;
	}

	return NULL;
}

static int __i2c_bus_lookup_by_name(const char *bus_name)
{
	int i = 0;
	int found_i2cbus = -1;
	struct i2c_adap *adapters;

	adapters = i2c_busses_gather();

	if (!adapters) {
		fprintf(stderr, "error[%s:%d] adapters == NULL!\n", __FILE__, __LINE__);
		return -3;
	}

	int bus_name_len = strlen(bus_name);
	while (adapters[i].name) {
		if (!strncmp(adapters[i].name, bus_name,bus_name_len)) {
			if (found_i2cbus >= 0) {
				fprintf(stderr, "error[%s:%d] Found same I2C bus name.\n", __FILE__, __LINE__);
				free_adapters(adapters);
				return -4;
			}
			found_i2cbus = adapters[i].nr;
		}
		i++;
	}

	if (found_i2cbus == -1) {
		fprintf(stderr, "error[%s:%d] I2C bus: %s not found!\n", __FILE__, __LINE__, bus_name);
	}

	free_adapters(adapters);
	return found_i2cbus;
}

int i2c_bus_lookup(const char *i2cbus_arg)
{
	unsigned long i2cbus;
	char *end;

	i2cbus = strtoul(i2cbus_arg, &end, 0);
	if (*end || !*i2cbus_arg) {
		return __i2c_bus_lookup_by_name(i2cbus_arg);
	}
	if (i2cbus > 0xFFFFF) {
		fprintf(stderr, "Error: I2C bus out of range!\n");
		return -2;
	}

	return i2cbus;
}

int i2c_address_parse(const char *address_arg)
{
	long address;
	char *end;

	address = strtol(address_arg, &end, 0);
	if (*end || !*address_arg) {
		fprintf(stderr, "Error: Chip address(%s) is not a number!\n",address_arg);
		return -1;
	}
	if (address < 0x03 || address > 0x77) {
		fprintf(stderr, "Error: Chip address(%s) out of range (0x03-0x77)!\n",address_arg);
		return -2;
	}

	return address;
}

int i2c_dev_open(int i2cbus, /*char *filename, size_t size, */int quiet)
{
	int fd;
	char file_path[20];
	size_t len = sizeof(file_path);

	snprintf(file_path, len, "/dev/i2c/%d", i2cbus);
	file_path[len - 1] = '\0';
	fd = open(file_path, O_RDWR);


	if (fd < 0) {
		if (errno == ENOENT || errno == ENOTDIR) {
			sprintf(file_path, "/dev/i2c-%d", i2cbus);
			fd = open(file_path, O_RDWR);
		}
	}

	if (fd < 0 && !quiet) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file `/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
				i2cbus, i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file `%s': %s\n", file_path, strerror(errno));
			if (errno == EACCES) {
				fprintf(stderr, "Run as root?\n");
			}
		}
	}

	return fd;
}

int slave_addr_set(int file, int address, int force)
{
	unsigned long request = force ? I2C_SLAVE_FORCE : I2C_SLAVE;

	if (ioctl(file, request, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n", address, strerror(errno));
		return -errno;
	}

	return 0;
}
