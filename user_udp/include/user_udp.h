#ifndef USER_UDP
#define USER_UDP

#define HELLO "HELLO"
#define OK "OK"
#define MAX_MESS_SIZE 6
#define MAX_UDP_SIZE 65507
#define MAX_RCV_WAIT 100000 // in microseconds
#define _100_MSEC 1000000


/* 0 is only one echo message HELLO-OK
 * 1 is the THROUGHPUT test (client send, server receives)
 * 2 is the LATENCY test, (multiple echo message HELLO-OK)
*/
// #define TEST 1

#endif
