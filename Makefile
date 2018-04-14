UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	obj-m += udp_client.o
	obj-m += udp_server.o

	K_DIR:= kern_udp

	COMMON_FILES:= $(K_DIR)/kernel_udp.o $(K_DIR)/k_file.o \
								$(K_DIR)/kernel_service.o $(K_DIR)/kernel_message.o

	udp_server-y:= \
		$(COMMON_FILES) \
		$(K_DIR)/udp_server.o \
		$(K_DIR)/kserver_operations.o

	udp_client-y:= \
		$(COMMON_FILES) \
		$(K_DIR)/udp_client.o \
		$(K_DIR)/kclient_operations.o

	# ADDITIONAL_FLAG:= #add here if you want to give flags to modules
	EXTRA_CFLAGS:=  -I$(PWD)/$(K_DIR)/include -O2 # $(ADDITIONAL_FLAG)
endif

USER_FLAGS:= -I$(PWD)/user_udp/include -O2

ccflags-y:= -std=gnu99 -Wno-declaration-after-statement

COMMON_OBJ:=user_udp.o user_message.o
COMMON_HDR:=user_udp.h user_message.h

I_DIR:= user_udp
USER_CL_OBJ:=user_client.o uclient_operations.o $(COMMON_OBJ)
USER_SERV_OBJ:=user_server.o userver_operations.o $(COMMON_OBJ)

_USER_CL_HEAD:= user_udp.h uclient_operations.h $(COMMON_HDR)
USER_CL_HEAD:= $(patsubst %,$(I_DIR)/include/%,$(_USER_CL_HEAD))

_USER_SERV_HEAD:= user_udp.h userver_operations.h $(COMMON_HDR)
USER_SERV_HEAD:= $(patsubst %,$(I_DIR)/include/%,$(_USER_CL_HEAD))

ifeq ($(UNAME), Linux)
all: user_client user_server
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
else
all: user_client user_server
endif

%.o: $(I_DIR)/%.c $(USER_CL_HEAD) $(USER_SERV_HEAD)
	$(CC) $(USER_FLAGS) $(ccflags-y) -c $< -o $@

user_client: $(USER_CL_OBJ)
	$(CC) $(USER_FLAGS) -o $@ $^

user_server: $(USER_SERV_OBJ)
	$(CC) $(USER_FLAGS) -o $@ $^

ifeq ($(UNAME), Linux)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm user_client user_server 
else
clean:
	rm user_client user_server *.o 
endif
