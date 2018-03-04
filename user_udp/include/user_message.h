#ifndef U_MESS
#define U_MESS

#define REQUEST "HELLO"
#define REPLY "OK"

typedef struct message_data message_data;
extern int MAX_MESS_SIZE;
extern message_data * request;
extern message_data *  reply;

extern message_data * create_rcv_message(void);
extern message_data * create_message(char * data, size_t size_data, int id);
extern int get_message_id(message_data * mess);
extern void set_message_id(message_data * mess, int id);
extern char * get_message_data(message_data * mess);
extern size_t get_message_size(message_data * mess);
extern size_t get_total_mess_size(message_data * mess);
extern void delete_message(message_data * mess);
extern void del_default_messages(void);
extern void init_default_messages();

#endif
