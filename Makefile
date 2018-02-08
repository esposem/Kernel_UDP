obj-m += udp_client.o
obj-m += udp_server.o

udp_server-y:= \
	k_udp/udp_server.o \
	k_udp/kernel_udp.o \
	k_udp/kserver_operations.o

udp_client-y:= \
	k_udp/udp_client.o \
	k_udp/kernel_udp.o \
	k_udp/kclient_operations.o

# ADDITIONAL_FLAG:= #add here if you want to give flags to modules
TEST_FLAG:= -D TEST=1
EXTRA_CFLAGS:=  -I$(PWD)/k_udp/include -O2  # $(ADDITIONAL_FLAG)

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	# cc $(TEST_FLAG) user_udp/user_client.c -o user_udp/user_client
	# cc $(TEST_FLAG) user_udp/user_server.c -o user_udp/user_server

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	# rm user_udp/user_client user_udp/user_server
