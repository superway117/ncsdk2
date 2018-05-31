
CXX = g++
CC = gcc


LDFLAGS += $(shell pkg-config --libs) -ldl -lm -L.
CFLAGS := -O2 -Wall -fPIC
INSTALLDIR := ${DESTDIR}/usr/local

LD_LIBRARY_PATH=/usr/local/lib



ifeq "$(wildcard $(INSTALLDIR) )" " "
	INSTALLDIR = ${DESTDIR}/usr
endif

all: libion.so.0

*.o: ion.h ion_kernel.h

.c.o:
	$(CC) -c $(CFLAGS) $<

libion.so.0: ion.o
	$(CC) -shared -o $@ -Wl,-h$@  ion.o $(LDFLAGS)

	ln -fs libion.so.0 libion.so


test: test.o
	$(CC) $(LDFLAGS) -o $@ test.o -lion -pthread

clean:
	rm -f *.o libion.so.0

basicinstall: libion.so.0
	mkdir -p $(INSTALLDIR)/lib/
	cp libion.so.0 $(INSTALLDIR)/lib/
	ln -fs libion.so.0 $(INSTALLDIR)/lib/libion.so
	mkdir -p $(INSTALLDIR)/include/
	cp ion.h $(INSTALLDIR)/include/
	cp ion_kernel.h $(INSTALLDIR)/include/

installpackage: basicinstall
	cp 99-hddl-ion.rules  $(INSTALLDIR)

rulesinstall:
	mkdir -p ${DESTDIR}/etc/udev/rules.d/
	cp 99-hddl-ion.rules ${DESTDIR}/etc/udev/rules.d/

install: basicinstall rulesinstall

uninstall:
	rm $(INSTALLDIR)/include/ion.h
	rm $(INSTALLDIR)/include/ion_kernel.h
	rm $(INSTALLDIR)/lib/libion.so.0
	rm $(INSTALLDIR)/lib/libion.so
	rm ${DESTDIR}/etc/udev/rules.d/99-hddl-ion.rules
