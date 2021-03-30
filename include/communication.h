#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define DEBUG 0

#define CMD_QUIT 1
#define CMD_SEND 2
#define CMD_FOLLOW 3
#define NOTIF 4
#define SRV_MSG 5
#define	INIT_USER 6 



#define BUFFER_SIZE 256

typedef struct packet{
 uint16_t type;         // Type of packet (CMD_QUIT, CMD_SEND, CMD_FOLLOW, NOTIF, SRV_MSG or INIT_USER)
 uint16_t sqn;          // Sequence number
 uint16_t len;       	// Payload length 
 uint16_t timestamp;    // Timestamp, in a "hhmm" format. E.g. 22h04min -> 2204, 2h15min -> 215
 char* payload;			// Message data
 } packet;

void send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload);

void receive_and_print(int sockfd);

void receive(int sockfd, packet* message);

int getTime();

#endif