obj-m += udp_client.o
obj-m += udp_server.o

udp_server-y:= \
	k_udp/udp_server.o \
	k_udp/kernel_udp.o

udp_client-y:= \
	k_udp/udp_client.o \
	k_udp/kernel_udp.o

	#  0 is only one echo message HELLO-OK
	#  1 is the THROUGHPUT test (client send, server receives)
	#  2 is the LATENCY test, (multiple echo message HELLO-OK)
TEST_FLAG:= -D TEST=2

EXTRA_CFLAGS:=  -I$(PWD)/k_udp/include $(TEST_FLAG)

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cc $(TEST_FLAG) user_udp/user_client.c -o user_udp/user_client
	cc $(TEST_FLAG) user_udp/user_server.c -o user_udp/user_server

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm user_udp/user_client user_udp/user_server
