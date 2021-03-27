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
#include <signal.h>

#include "utils.c"
#include "profile.c"
#include "notification.c"

#define PORT 4000



int sqncnt = 0;
int sockfd;
profile user_list[MAX_CLIENTS];

//detecting ctrl+c
void intHandler(int dummy) {
   close(sockfd);   
   printf("\nServer ended successfully\n");
   exit(0);
}



int handle_profile(char *username){

   int profile_id = get_profile_id(user_list,username);

   if(profile_id == -1){
      profile new_profile;
      new_profile.name = username;
      new_profile.online = 1;
      profile_id = insert_profile(user_list, new_profile);

      if(profile_id == -1){
         printf("MAX NUMBER OF PROFILES REACHED\n");
         exit(1);
      }
   } 
   else{
      if(user_list[profile_id].online > 1){
         //TODO
         printf("ESSE USUARIO NÃO É PERMITIDO\n");
      }
      else{
         user_list[profile_id].online +=1;
      }
   }

   return profile_id;

}

void *handle_client(void *arg) {

   // TO-DO: Login code. profile.online++.

   int newsockfd = *(int *) arg;
   int flag = 1;
   packet message;


   int profile_id;

   signal(SIGINT, intHandler); //detect ctrl+c
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

         case INIT_USER:
            printf("%s\n", message.payload);
            profile_id = handle_profile(message.payload);
            print_profiles(user_list);

         break;
      }

      free(message.payload);
   }

}


int main( int argc, char *argv[] ) {

   pthread_t client_pthread[MAX_CLIENTS];
   int newsockfd, portno, clilen;
   int yes =1;
   struct sockaddr_in serv_addr, cli_addr;
   
   init_profiles(user_list);
   

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

   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
       perror("setsockopt");
       exit(1);
   }
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
      signal(SIGINT, intHandler); //detect ctrl+c
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