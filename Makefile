obj-m += udp_client.o
obj-m += udp_server.o

udp_server-objs:= \
	kpaxos/udp_server.o \
	kpaxos/kernel_udp.o

udp_client-objs:= \
	kpaxos/udp_client.o \
	kpaxos/kernel_udp.o

EXTRA_CFLAGS:=  -I$(PWD)/kpaxos/include

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
