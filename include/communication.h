#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define DEBUG 1

#define CMD_QUIT 1
#define CMD_SEND 2
#define CMD_FOLLOW 3
#define NOTIF 4
#define SRV_MSG 5
#define	INIT_USER 6 

//Communication between RMs
#define ACK 7
#define INIT_BACKUP 8
#define CREATE_USER 9
#define ADD_ONLINE 10
#define SUB_ONLINE 11
#define NOTIF_CONSUMED 12
#define UPDATE_PORT 13
#define HEARTBEAT 14





#define BUFFER_SIZE 256

typedef struct packet{
 uint16_t type;         // Type of packet (CMD_QUIT, CMD_SEND, CMD_FOLLOW, NOTIF, SRV_MSG or INIT_USER)
 uint16_t sqn;          // Sequence number
 uint16_t len;       	// Payload length 
 uint16_t timestamp;    // Timestamp, in a "hhmm" format. E.g. 22h04min -> 2204, 2h15min -> 215
 uint16_t userid;       //  Used only for communication between RMs, to indicate what user will do what operation
 char* payload;			// Message data
 } packet;

int send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload);
int send_packet_with_userid(int sockfd, int userid, int type, int sqn, int len, int timestamp, char* payload);
void receive_and_print(int sockfd);

int receive(int sockfd, packet* message);

int getTime();

#endif