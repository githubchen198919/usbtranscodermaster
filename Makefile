
CC=arm-linux-gnueabihf-gcc
CFLAGS=-g -Wall

target=usbtranscodermaster_app
 
all:$(target)
	
$(target):
	mkdir -p bin
	cd src ; $(CC) -std=c99 -D_GNU_SOURCE $(CFLAGS) -D HOST_APP -o ../bin/$(target)	\
	-I ../include	\
	-I ../include/libusb-1.0 \
	-I ../include/vdec \
	usbtranscodermaster.c \
	usbtransfer.c \
	vdec.c \
	file_op.c \
	-l usb-1.0 \
	-l pthread \
	-L ../lib \
 
.PHONY:
	clean
 
clean:
	rm *.o $(target) -rf
