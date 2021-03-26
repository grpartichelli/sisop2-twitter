#include <time.h>

#define CMD_QUIT 0
#define CMD_SEND 1
#define CMD_FOLLOW 2
#define NOTIF 3
#define SRV_MSG 4

#define BUFFER_SIZE 256

typedef struct packet{
 uint16_t type;         // Type of packet (CMD_QUIT, CMD_LOGIN, CMD_SEND, CMD_FOLLOW, DATA_NOTIF or SRV_MSG)
 uint16_t sqn;          // Sequence number
 uint16_t len;       	// Payload length 
 uint16_t timestamp;    // Timestamp, in a "hhmm" format. E.g. 22h04min -> 2204, 2h15min -> 215
 } packet;

void send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload);

void receive_and_print(int sockfd);

void receive(int sockfd, packet* message, char* text);

int getTime();
