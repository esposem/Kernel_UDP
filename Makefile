obj-m += udp_client.o
obj-m += udp_server.o

udp_server-y:= \
	kern_udp/udp_server.o \
	kern_udp/kernel_udp.o \
	kern_udp/kserver_operations.o

udp_client-y:= \
	kern_udp/udp_client.o \
	kern_udp/kernel_udp.o \
	kern_udp/kclient_operations.o

# ADDITIONAL_FLAG:= #add here if you want to give flags to modules
USER_FLAGS:= -I$(PWD)/user_udp/include -O2
EXTRA_CFLAGS:=  -I$(PWD)/kern_udp/include -O2 -D TIMER # $(ADDITIONAL_FLAG)

ccflags-y:= -std=gnu99 -Wno-declaration-after-statement

I_DIR:= user_udp
USER_CL_OBJ:=user_client.o user_udp.o uclient_operations.o
USER_SERV_OBJ:=user_server.o user_udp.o userver_operations.o

_USER_CL_HEAD:= user_udp.h uclient_operations.h
USER_CL_HEAD:= $(patsubst %,$(I_DIR)/include/%,$(_USER_CL_HEAD))

_USER_SERV_HEAD:= user_udp.h userver_operations.h
USER_SERV_HEAD:= $(patsubst %,$(I_DIR)/include/%,$(_USER_CL_HEAD))

all: user_client user_server
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

%.o: $(I_DIR)/%.c $(USER_CL_HEAD) $(USER_SERV_HEAD)
	$(CC) $(USER_FLAGS) $(ccflags-y) -c $< -o $@

user_client: $(USER_CL_OBJ)
	$(CC) $(USER_FLAGS) -o $@ $^

user_server: $(USER_SERV_OBJ)
	$(CC) $(USER_FLAGS) -o $@ $^

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm user_client user_server
