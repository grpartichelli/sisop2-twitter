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
profile profile_list[MAX_CLIENTS];

//detecting ctrl+c
void intHandler(int dummy) {
   close(sockfd);   
   printf("\nServer ended successfully\n");
   exit(0);
}



int handle_profile(char *username, int newsockfd){

   int profile_id = get_profile_id(profile_list,username);

   if(profile_id == -1){
      
      profile_id = insert_profile(profile_list, username, 1);

      if(profile_id == -1){
         printf("MAX NUMBER OF PROFILES REACHED\n");
         exit(1);
      }
   } 
   else{
      if(profile_list[profile_id].online > 1){
         //TODO
         printf("Um usuario tentou exceder o numero de acessos.\n");
         send_packet(newsockfd,CMD_QUIT,++sqncnt,4,0,"quit");
         close(newsockfd);
      }
      else{
      
         profile_list[profile_id].online +=1;
      }
   }

   return profile_id;

}

void *handle_client(void *arg) {

   

   int newsockfd = *(int *) arg;
   int flag = 1;
   int profile_id;
   packet message;

   char follow_name[21];
   char payload[40];
   int follow_id;


   signal(SIGINT, intHandler); //detect ctrl+c
   while(flag){
      
      //READ
      receive(newsockfd, &message);

      switch(message.type)
      {
         case CMD_QUIT:
         // TO-DO: QUIT command. profile.online--, send a packet (SRV_MSG), close the socket. 
            
            send_packet(newsockfd,SRV_MSG,++sqncnt,1,0,"");
            profile_list[profile_id].online -=1;
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
            strcpy(follow_name,message.payload);
            printf("FOLLOW NAME: %s\n",follow_name); 
            follow_id = get_profile_id(profile_list,follow_name);

            if(follow_id == -1){ //User doesnt exist
               strcpy(payload,"FOLLOW falhou, usuario nao encontrado.\n");
               send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
               
            }
            else{
               strcpy(payload,"FOLLOW executou com sucesso.\n");
               send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload);  
            }

         // TO-DO: FOLLOW command. verify whether the @ exists, if it doesn't send an error message to the client (SRV_MSG)
         // if it does, add the current user to the @'s list of followers and send the client an empty message (len=0).

         break;

         case INIT_USER:
            

            profile_id = handle_profile(message.payload,newsockfd);

            
            

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
   
   init_profiles(profile_list);
   

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