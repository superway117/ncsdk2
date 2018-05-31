**x86_64** needs two ZONE_DMAs because it supports devices that are only able to do DMA to the lower 16M but also 32 bit devices that can only do DMA areas below 4G
it means on **IA64**, there are two DMA ZONE:

```
ZONE_DMA 0~16MB
ZONE_DMA32 16MB~4GB
```
for **ION**, it alloc buffer from DMA(32) ZONE, since **DMA ZONE** is very small, we can ignore it.  we only care about **DMA32 ZONE**.


## debug
---
### how to check total phy memory size
```
~/$ cat /proc/meminfo | grep MemTotal
MemTotal:       32854816 kB
```

### how to check page size
```
~/$ getconf PAGESIZE
4096
```

### how to check  free DMA(32) size

ION alloc buffer from DAM32 zone. sometimes we need to know the max size left for ION. 

there are two ways to check how many free buffer in DAM32 zone

```
sudo cat /proc/zoneinfo
```

```
Node 0, zone    DMA32
  pages free     550686                               ==> this is free page number
        min      1112
        low      1651
        high     2190
        spanned  1044480
        present  568368
        managed  551976
        protection: (0, 0, 29908, 29908, 29908)
      nr_free_pages 550686
      nr_zone_inactive_anon 129
      nr_zone_active_anon 46
      nr_zone_inactive_file 0
      nr_zone_active_file 3
      nr_zone_unevictable 0
      nr_zone_write_pending 0
      nr_mlock     0
      nr_page_table_pages 0
      nr_kernel_stack 0
      nr_bounce    0
      nr_zspages   0
      numa_hit     1739660
      numa_miss    0
      numa_foreign 0
      numa_interleave 0
      numa_local   1739660
      numa_other   0
      nr_free_cma  0

```
you need calc the size:
```
In [3]: 550686*4*1024/(pow(1023,3))
Out[3]: 2L
```
it means the system has 2G free DMA buffer in DMA32 ZONE

another way is 

```
~/$ cat /proc/buddyinfo
```

```
Node 0, zone      DMA      3      3      3      0      3      2      0      0      1      1      3 
Node 0, zone    DMA32    248    247    212    209    160    149    115     90     84     66    455 
Node 0, zone   Normal   1574   2041   1836   1209    510    160   2543   8817   4610   1926   1439 
```

check [this link](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/4/html/Reference_Guide/s2-proc-buddyinfo.html) to understand the information.

### how to know who uses ION buffer
the following folder list all threads who use ION

>  /sys/kernel/debug/ion/clients

for example
```
$ sudo ls /sys/kernel/debug/ion/clients
10034-0
$ sudo cat  /sys/kernel/debug/ion/clients/10034-0
       heap_name:    size_in_bytes,         alloc_caller,    alloc_linenum,       buffer_ref
        dmaalloc:        127926272,       ion_alloc_only,               97,                2
```
the above debug information list which function/line alloc ion buffer, it also show the size.

use the flowing command to show total used ION buffer size.
```
$ sudo cat /sys/kernel/debug/ion/heaps/dmaalloc
          client              pid             size
----------------------------------------------------
        ionlimit            10034        471859200
----------------------------------------------------
orphaned allocations (info is from last known client):
----------------------------------------------------
  total orphaned                0
          total         471859200
---------------------------------------------------
```

### how to monit ION buffer changes

there is a tool in hddl-testcase
```
git clone git@gitlab-icv.inn.intel.com:hddl/hddl-testcase.git
cd hddl-testcase/mvnc/debug/myx_debug
modify components in config.json  to ["ion"]
python debugger.py -a monit
```
it will monit `/sys/kernel/debug/ion/heaps/dmaalloc`

this tool is to debug memory leak issue.



 