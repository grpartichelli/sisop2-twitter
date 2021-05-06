#include "../include/communication.h"


void send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload)
{
	packet message;
	message.type = type;
	message.sqn = sqn;
	message.len = len;
	message.timestamp = timestamp;
	message.userid = -1;
	

	//if(DEBUG)
	//	printf("Enviado %i, %i, %i, %i, %s.\n", message.type, message.sqn, message.len, message.timestamp, payload);

	write(sockfd,&message,10);
	
	write(sockfd,payload,strlen(payload));


}

void send_packet_with_userid(int sockfd, int userid, int type, int sqn, int len, int timestamp, char* payload)
{
	packet message;
	message.type = type;
	message.sqn = sqn;
	message.len = len;
	message.timestamp = timestamp;
	message.userid = userid;
	
	

	//if(DEBUG)
	//	printf("Enviado %i, %i, %i, %i, %s.\n", message.type, message.sqn, message.len, message.timestamp, payload);

	write(sockfd,&message,10);
	
	write(sockfd,payload,strlen(payload));


}


void receive_and_print(int sockfd)
{
	packet message;

	receive(sockfd,&message);

	if(message.payload!=NULL && message.len!=0)
	{
		printf("%s\n", message.payload);
		free(message.payload);
	}
}

void receive(int sockfd, packet* message)
{

	while(read(sockfd,message,10) < 0)
    	;

    if(message->len!=0)
    {
    	message->payload = (char*) malloc((message->len)*sizeof(char));
    	read(sockfd,message->payload,message->len);
    	message->payload[message->len-1]='\0';
	}
	else
		message->payload=NULL;

	if(DEBUG)
		printf("Recebido %i, %i, %i, %i, %s\n", message->type, message->sqn, message->len, message->timestamp, message->payload);

}

int getTime()
{
	
	time_t now = time(NULL);
	struct tm *tm_struct = localtime(&now);

	int hour = tm_struct->tm_hour;
	int minute = tm_struct->tm_min;

	return hour*100+minute;
}

