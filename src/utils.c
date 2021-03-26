#include "../include/utils.h"
#include <string.h>

void send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload)
{
	packet message;
	message.type = type;
	message.sqn = sqn;
	message.len = strlen(payload);
	message.timestamp = timestamp;

	printf("Enviado %i, %i, %i, %i, %s", message.type, message.sqn, message.len, message.timestamp, payload);

	write(sockfd,&message,8);
	if (type)
		write(sockfd,payload,strlen(payload));
}

void receive_and_print(int sockfd)
{
	packet message;
	char* text;

	receive(sockfd,&message,text);

	if(message.len)
		printf("%s", text);
}

void receive(int sockfd, packet* message, char* text)
{
	printf("oi");

	while(read(sockfd,message,8) < 0)
    	;

    if(message->len)
    {
    	text = (char*) malloc((message->len)*sizeof(char));
    	read(sockfd,text,message->len);
    	text[message->len-1]='\0';
	}

	printf("Recebido %i, %i, %i, %i, %s\n", message->type, message->sqn, message->len, message->timestamp, text);

}

int getTime()
{
	// TO-DO: convert sth from the "time.h" library into a timestamp

	return 0;
}