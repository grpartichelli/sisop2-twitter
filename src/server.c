#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "utils.c"

#define PORT 4000
#define MAX_CLIENTS 500
#define MAX_NOTIFS 500

int sqncnt = 0;

// Gerenciador de comunicação: OK?
// Gerenciador de notificações: TO-DO
// Gerenciador de perfis e sessões: TO-DO

typedef struct notification{
 uint32_t id;           // Notification identifier (int, unique)
 uint32_t timestamp;    // Timestamp
 const char* msg;       // Message
 uint16_t len;          // Message length
 uint16_t pending;      // Number of pending readers
 } notification;

typedef struct profile{
 char* id;              // Profile identifier (@(...))
 int online;            // Number of sessions open with this specific user
 char** followers;      // List of followers
 notification* rcv_notifs[MAX_NOTIFS]; // List of received notifs
 notification* pnd_notifs[MAX_NOTIFS]; // List of pending notifs
 } profile;

void *handle_client(void *arg) {

   // TO-DO: Login code. profile.online++.

   int newsockfd = *(int *) arg;
   int flag = 1;

   packet message;

   while(flag){
           
      //READ
      receive(newsockfd, &message);

      switch(message.type)
      {
         case CMD_QUIT:
         // TO-DO: QUIT command. profile.online--, send a packet (SRV_MSG), close the socket. 
            send_packet(newsockfd,SRV_MSG,++sqncnt,1,0,"");
            close(newsockfd);
            flag = 0;
         break;

         case CMD_SEND:
         // TO-DO: SEND command.
         /* a “produção” de uma notificação envolverá 
            (1) receber a notificação do processo cliente, 
            (2) escrever a notificação na lista de notificações pendentes de envio e 
            (3) para cada seguidor, atualizar a fila de notificações pendentes de recebimento */
         break;

         case CMD_FOLLOW:
         // TO-DO: FOLLOW command. verify whether the @ exists, if it doesn't send an error message to the client (SRV_MSG)
         // if it does, add the current user to the @'s list of followers and send the client an empty message (len=0).

         break;
      }

      free(message.payload);
   }

}


int main( int argc, char *argv[] ) {

   pthread_t client_pthread[MAX_CLIENTS];
   int sockfd, newsockfd, portno, clilen;
   struct sockaddr_in serv_addr, cli_addr;
   
   //Create socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      printf("ERROR opening socket\n");
      exit(1);
   }
   
   //Initializing structure
   bzero((char *) &serv_addr, sizeof(serv_addr));
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(PORT);

   // BIND TO HOST
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      printf("ERROR on binding.\n");
      exit(1);
   }

   printf("Server started correctly.\n");

	//LISTEN
   listen(sockfd,5);
   clilen = sizeof(cli_addr);

   int i=0;
   while(1){
      
      //ACCEPT
      newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
      if (newsockfd < 0) {
         printf("ERROR on accept");
         exit(1);
      }

      if(pthread_create(&client_pthread[i], NULL, handle_client, &newsockfd) != 0 ){
         printf("Failed to create thread");
         exit(1);
      }
      
      i++;
   }
	  
   close(sockfd);
   printf("Server ended successfully\n");

   return 0;
}