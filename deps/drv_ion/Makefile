

GZ_NAME := myd_ion_v`modinfo --field version myd_ion.ko`.tar.gz





obj-m := myd_ion.o
myd_ion-objs = compat_ion.o		\
		ion.o			\
		ion_cma_heap.o		\
		ion_system_heap.o		\
		ion_heap.o	\
		ion_page_pool.o		\
		ion_driver.o	\
		plist.o

ifeq ($(shell uname -r | cut -d- -f2),yocto)
	YOCTO :=1
else ifeq ($(shell cat /etc/redhat-release | cut -d' ' -f1),CentOS)
  CentOS :=1
endif


ifeq ($(YOCTO),1)
  KDIR := /usr/src/kernel
	ccflags-y += -DYOCTO
else ifeq ($(CentOS),1)
  KDIR := /lib/modules/`uname -r`/build
	ccflags-y += -DCentOS
	myd_ion-objs += ion_carveout_heap.o
	myd_ion-objs += ion_chunk_heap.o
else
  KDIR := /lib/modules/`uname -r`/build
	myd_ion-objs += ion_carveout_heap.o
	myd_ion-objs += ion_chunk_heap.o
endif

ccflags-y += -O2 -Wno-pointer-to-int-cast 

all:
	@echo ">>> start building NODEBUG ION driver <<<"
	@$(MAKE) -C $(KDIR)  M=$$PWD
	@echo ">>> end building NODEBUG ION driver <<<\n"

debug:
	@echo ">>> start building DEBUG ION driver <<<"
	@echo $(VERSION)
	@$(MAKE) -C $(KDIR) M=$$PWD "ccflags-y+=$(ccflags-y) -DDEBUG"
	@echo ">>> end building DEBUG ION driver <<<\n"

debug_all:
	@echo ">>> start building DEBUG ION driver <<<"
	@echo $(VERSION)
	@$(MAKE) -C $(KDIR) M=$$PWD "ccflags-y+=$(ccflags-y) -DDEBUG"
	@echo ">>> end building DEBUG ION driver <<<\n"

clean:
	@rm -fr .built-in.o.cmd myd_ion.ko *.o .myd_ion* modules.order *~
	@rm -fr Module.symvers myd_ion.mod.c modules.order .tmp_versions .*.o.cmd


TARGET := myd_ion.ko
$(TARGET): all

.PHONY: preuninstall
preuninstall:
	if [[ "$(shell lsmod | grep $(basename $(TARGET)))" != "" ]]; then sudo rmmod $(basename $(TARGET)); fi;
	@make clean

.PHONY: preinstall
preinstall: preuninstall
	@make
	if [[ "$(shell lsmod | grep $(basename $(TARGET)))" != "" ]]; then sudo rmmod $(basename $(TARGET)); fi;
	@insmod $(TARGET)
	@modinfo $(TARGET)

.PHONY: install
install: SHELL=/bin/bash

.PHONY: uninstall
uninstall: SHELL=/bin/bash

ifeq ($(shell lsb_release -si),Ubuntu)
CONF_PATH=/etc/modules-load.d/$(basename $(TARGET)).conf
DRV_PATH=/lib/modules/$(shell uname -r)/kernel/drivers/myd/$(TARGET)
endif

install: preinstall
ifeq ($(shell lsb_release -si),Ubuntu)
	mkdir -p $(dir $(DRV_PATH))
	cp $(TARGET) $(DRV_PATH)
	sudo depmod
	@modinfo $(basename $(TARGET))
	mkdir -p /etc/modules-load.d/
	@echo -e "#Load " $(TARGET) " at boot\n"$(basename $(TARGET)) > $(CONF_PATH)
	cat $(CONF_PATH)
	@echo -e " \e[1m" "\e[96m" "Driver $(TARGET) is installed to $(DRV_PATH)" "\e[0m"
	@echo -e " \e[1m" "\e[96m" "$(CONF_PATH) is created for auto-load at boot time" "\e[0m"
endif

uninstall: preuninstall
ifeq ($(shell lsb_release -si),Ubuntu)
	[ ! -e $(CONF_PATH) ] || rm $(CONF_PATH)
	[ ! -e $(DRV_PATH) ] || rm $(DRV_PATH)
	if [[ `ls -A $(dir $(DRV_PATH))` == "" ]]; then sudo rm -rf $(dir $(DRV_PATH)); fi;
	sudo depmod
	@echo -e " \e[1m" "\e[96m" "$(DRV_PATH) is removed" "\e[0m"
	@echo -e " \e[1m" "\e[96m" "$(CONF_PATH) is removed" "\e[0m"
endif



release:
	@echo "\n>>> packing sources & scripts to $(GZ_NAME) <<<\n"
	@make clean

help:
	@echo ""
	@echo "make clean   -> delete build artefacts"
	@echo "make         -> build a non-debug driver"
	@echo "make debug   -> build a debug driver"
	@echo "make install -> install a new driver (build driver and print driver info)"
	@echo "make release -> to generate a release zip"
	@echo ""

