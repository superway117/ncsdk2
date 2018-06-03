CC=gcc
LIBUSB_INCLUDE = /usr/include/libusb-1.0
LIB_PATH = /usr/lib
CFLAGS=-Wall
CFLAGS += -I$(LIBUSB_INCLUDE)
CFLAGS += -L$(LIB_PATH)
CFLAGS += -lpthread
LIBUSB_LIBS = usb-1.0

VSC_OBJ = vscHost.o 

all: vscHost 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

vscHost: $(VSC_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -l$(LIBUSB_LIBS)



.PHONY: clean all
.DEFAULT_GOAL=all

clean:
	rm vscHost.o
	rm vscHost
