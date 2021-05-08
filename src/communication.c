#include "../include/communication.h"


int send_packet(int sockfd, int type, int sqn, int len, int timestamp, char* payload)
{
	int err = 0;
	socklen_t size = sizeof(int);
	int is_connected = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &err, &size);
	if (is_connected == 0 && type >=0 && type <= HEARTBEAT) 
	{
	//	printf("it's connected!\n");
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
		return 1;
	}
	//printf("it's not connected!\n");
	return 0;
}

int send_packet_with_userid(int sockfd, int userid, int type, int sqn, int len, int timestamp, char* payload)
{
	int err = 0;
	socklen_t size = sizeof(int);
	int is_connected = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &err, &size);
	if (is_connected == 0) 
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
		return 1;
	}
	return 0;

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

int receive(int sockfd, packet* message)
{
	int read_ = 10;
	int err = 0;
	socklen_t size = sizeof(int);
	int is_connected = getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &err, &size);
	if (is_connected == 0) 
	{
		do
		{
			read_ = read(sockfd,message,10);
		}
		while(read_<0);

		if(read_ == 0)
			return 0;

    	if(message->len!=0)
    	{
    		message->payload = (char*) malloc((message->len)*sizeof(char));
    		read(sockfd,message->payload,message->len);
    		message->payload[message->len-1]='\0';
			//if(DEBUG)
				//printf("Recebido %i, %i, %i, %i, %s\n", message->type, message->sqn, message->len, message->timestamp, message->payload);
		}
		else
			message->payload=NULL;
		return 1;
	}
	return 0;

}

int getTime()
{
	
	time_t now = time(NULL);
	struct tm *tm_struct = localtime(&now);

	int hour = tm_struct->tm_hour;
	int minute = tm_struct->tm_min;

	return hour*100+minute;
}

