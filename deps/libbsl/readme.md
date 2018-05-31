# libbsl
---
this project includes some tools and a library(shared library).

1. `libbsl`: it is a library for hddl smbus support both mcu and ioexpander 
2. `bsl_reset`: it is a tool to reset devices based on `libbsl`

## make&install

it depends on json-c, so need install json-c before make libbsl.

```
$ git clone https://github.com/json-c/json-c.git
$ cd json-c
$ sh autogen.sh
$ ./configure  # --enable-threading
$ make
$ make install
```
then compile `libbsl`
---
`Use make`
```
make clean
make
sudo make install
```

`Use Cmake`
```
mkdir build
cd build
cmake ..
make -j
sudo make install
```

## uninstall

`Use make`
```
make uninstall
```
`Use Cmake`
```
xargs rm < install_manifest.txt
```
 
## interfaces

all interfaces list here
> include/hddl-i2c.h

`bsl_reset` is based on libbsl


## how to know the i2c address and write config file
---
better we can fix the i2c addres in config file to avoid confusion

the config file path is
> ${HDDL_INSTALL_DIR}/config/bsl.json

---
this is a example how to config the i2c address, one address means one pcie card.

```
for ioexpander:

{
  "active":"ioexpander",
  "i2c_addr":[37,39]
}


for mcu:

{
  "active":"mcu",
  "i2c_addr":[31]
}

```

### MCU case

the following steps to get the MCU i2c address(one pcie card) and write the config file

- install i2c-i801 driver

  ```
  sudo modprobe i2c-i801
  ```
  
- list all i2c devices

  ```
  i2cdetect -l

  hddl@hddl-US-E2332:~/eason/hddl-bsl/src$ i2cdetect -l
  i2c-0   i2c             i915 gmbus dpc                          I2C adapter
  i2c-1   i2c             i915 gmbus dpb                          I2C adapter
  i2c-2   i2c             i915 gmbus dpd                          I2C adapter
  i2c-3   i2c             DPDDC-C                                 I2C adapter
  i2c-4   i2c             DPDDC-E                                 I2C adapter
  i2c-5   smbus           SMBus I801 adapter at f040              SMBus adapter  ==>
  ```
we know i2c-5 is the right one, then check the i2c address

- check i2c address

the following case is the MCU example, the address range is from 0x18 to 0x1f, 
the 0x18 is always there, so we know it is not the pcie card, 0x1f is the i2c address for pcie card.

  ```
  i2cdetect -y 5

  hddl@hddl-US-E2332:~/eason/hddl-bsl/src$ i2cdetect -y 5
       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
  00:          -- -- -- -- -- 08 -- -- -- -- -- -- --
  10: -- -- -- -- -- -- -- -- 18 -- -- -- -- -- -- 1f                   ==>
  20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  30: 30 31 -- -- 34 35 36 -- -- -- -- -- -- -- -- --
  40: -- -- -- -- 44 -- -- -- -- -- -- -- -- -- -- --
  50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  70: -- -- -- -- -- -- -- --

  ```
0x1f == 31, so  config the file like the following , then it is enough.

```
{
  "active":"mcu",
  "i2c_addr":[31]
}
```


### IOExpander Case


the following steps to get the  IOExpander  i2c address(2 cards) and write the config file
- install i2c-i801 driver
    
    ```
    sudo modprobe i2c-i801
    ```
    
- list all i2c devices

    ```
    i2cdetect -l
    
    hddl@hddl-US-E2332:~/eason/hddl-bsl/src$ i2cdetect -l
    i2c-0   i2c             i915 gmbus dpc                          I2C adapter
    i2c-1   i2c             i915 gmbus dpb                          I2C adapter
    i2c-2   i2c             i915 gmbus dpd                          I2C adapter
    i2c-3   i2c             DPDDC-C                                 I2C adapter
    i2c-4   i2c             DPDDC-E                                 I2C adapter
    i2c-5   smbus           SMBus I801 adapter at f040              SMBus adapter  ==>
    ```
    
we know i2c-5 is the right one, then check the i2c address

- check i2c address

the following case is the IOExpander example, the address range is from 0x20 to 0x2f. there are two cards, so we know the i2c  address is 0x23 and 0x26.
 
```
i2cdetect -y 5

hddl@hddl-US-E2332:~$ i2cdetect -y 5
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- 08 -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- 18 -- -- -- -- -- -- --
20: -- -- -- 23 -- -- 26 -- -- -- -- -- -- -- -- --
30: 30 31 -- -- 34 35 36 -- -- -- -- -- -- -- -- --
40: -- -- -- -- 44 -- -- -- -- -- -- -- -- -- -- --
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 6f

```

0x23 == 35, 0x26==38, so  config the file like the following , then it is enough.


```

{
  "active":"ioexpander",
  "i2c_addr":[37,39]
}

```

 
## how to reset  IOExpander 

use i2c-tools

0x23   is the i2c address

--> reset all

```
sudo i2cset -y 5 0x23  0x06 0x00 b
sudo i2cset -y 5 0x23 0x02 0x00 b
sudo i2cset -y 5 0x23 0x02 0xff b

```

use bsl_reset

```
bsl_reset -d io 
```

-->reset one device
 
use bsl_reset
0x23-0x20 == 0x3, it is board id, 011
device id is from 0-7, use 1 as example
b 011 00001 == 0x61 == 97

```
bsl_reset -d io -i  97
```

## how to reset  MCU

use i2c-tools
0x1f is the i2c address

--> reset all

```
sudo i2cset -y 5 0x1f  0x01 0xff b
```

use bsl_reset

```
bsl_reset -d mcu
```

--> reset one device

0x2 is the device id

```
sudo i2cset -y 5 0x1f  0x01 0x2 b
```

use bsl_reset

```
bsl_reset -d mcu -i 2
```


## cmds
---

```
lab_seghwusr@SDIL16D016:~/eason/libbsl/src$ i2cdetect -y   6
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- 08 -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 1f 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: 30 31 -- -- 34 35 36 -- -- -- -- -- -- -- -- 3f 
40: -- -- -- -- 44 -- -- -- -- -- -- -- -- -- -- -- 
50: 50 -- 52 -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- 6c -- -- -- 
70: -- -- -- -- -- -- -- --                         
```
