INSTALLDIR := ${DESTDIR}/usr/local
LIBUSB_CFLAGS := $(shell pkg-config --cflags)
LDFLAGS += $(shell pkg-config --libs) -ldl -ljson-c
CFLAGS += -O2 -Wall $(LIBUSB_CFLAGS) -I../include -fPIC



OBJS = i2cbusses.o smbus_linux.o mcu.o ioexpander.o main.o bsl_cfg.o



ifeq "$(wildcard $(INSTALLDIR) )" " "
	INSTALLDIR = ${DESTDIR}/usr
endif

all: libbsl.so.0 tool
tool: bsl_reset

*.o: ../include/i2c-dev.h ../include/hddl-i2c.h i2cbusses.h

.c.o:
	$(CC) -c  $(CFLAGS) $<

libbsl.so.0: $(OBJS)
	$(CC) -shared -o $@ -Wl,-h$@ $(CFLAGS)  $(OBJS) $(LDFLAGS)
	ln -fs libbsl.so.0 libbsl.so

clean:
	rm -f *.o libbsl.so.0 libbsl.so bsl_reset

bsl_reset: bsl_reset.o
	$(CC) $(LDFLAGS) -o $@ bsl_reset.o  -L./ -lbsl

basicinstall: libbsl.so.0
	mkdir -p $(INSTALLDIR)/include/
	mkdir -p $(INSTALLDIR)/lib/
	mkdir -p $(INSTALLDIR)/bin/
	cp libbsl.so.0 $(INSTALLDIR)/lib/
	ln -fs libbsl.so.0 $(INSTALLDIR)/lib/libbsl.so
	cp ../include/* $(INSTALLDIR)/include/
	cp bsl_reset $(INSTALLDIR)/bin/
	cp 98-hddlbsl.rules ${DESTDIR}/etc/udev/rules.d/

postinstall:
	modprobe i2c-i801
	udevadm control --reload-rules
	udevadm trigger
	ldconfig

install: basicinstall postinstall

uninstall:
	rm $(INSTALLDIR)/lib/libbsl.so.0
	rm $(INSTALLDIR)/lib/libbsl.so
	rm $(INSTALLDIR)/include/i2c-dev.h
	rm $(INSTALLDIR)/include/hddl-i2c.h
