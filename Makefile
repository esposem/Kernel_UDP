obj-m += udp_client.o
obj-m += udp_server.o

udp_server-objs:= \
	k_udp/udp_server.o \
	k_udp/kernel_udp.o

udp_client-objs:= \
	k_udp/udp_client.o \
	k_udp/kernel_udp.o

EXTRA_CFLAGS:=  -I$(PWD)/k_udp/include

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
