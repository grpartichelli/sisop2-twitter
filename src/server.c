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
      
      profile_id = insert_profile(profile_list, username);

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
void handle_follow(char *follow_name, int profile_id, int newsockfd){

   
   char payload[100];
   int follow_id;
   int num_followers;


   follow_id = get_profile_id(profile_list,follow_name);

   if(follow_id == -1){ //User doesnt exist
      strcpy(payload,"FOLLOW falhou, usuario nao encontrado.\n");
      send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
      return;
   }

   if(strcmp(follow_name,profile_list[profile_id].name) == 0 ){
      strcpy(payload,"FOLLOW falhou, voce nao pode se seguir.\n");
      send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
      return;
   }



   num_followers =  profile_list[follow_id].num_followers;

   if(num_followers >= MAX_FOLLOW){
      printf("Account reached max number of followers\n");
      exit(1);
   }

   profile_list[follow_id].num_followers++;

   profile_list[follow_id].followers[num_followers] =  &profile_list[profile_id];

   strcpy(payload,"FOLLOW executou com sucesso.\n");
   send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload);  


}

void handle_send(notification *notif, packet message, int profile_id, int newsockfd){

   
   profile *p;
   int num_pnd_notifs;
   int num_followers = profile_list[profile_id].num_followers;
   int notif_id;
   
   //Update notif id
   notif_id = profile_list[profile_id].num_snd_notifs;
   profile_list[profile_id].num_snd_notifs++;

   //Create notification
   notif =  malloc(sizeof(notif));
   notif->id = notif_id;
   notif->timestamp = message.timestamp;
   notif->msg = message.payload;
   notif->len = message.len;
   notif->pending= profile_list[profile_id].num_followers;



   //Putting the notification on the current profile as send
   profile_list[profile_id].snd_notifs[notif_id] = notif;

   
   //Putting the notification of followers pending list
   for(int i=0; i< num_followers;i++){

      p = profile_list[profile_id].followers[i];

      num_pnd_notifs = p->num_pnd_notifs; 
      p->num_pnd_notifs++;

      p->pnd_notifs[num_pnd_notifs].notif_id= notif_id;
      p->pnd_notifs[num_pnd_notifs].profile_id= profile_id;
   
   }



   

}
  

void *handle_client_messages(void *arg) {

   

   int newsockfd = *(int *) arg;
   int flag = 1;
   int profile_id;
   packet message;

   char follow_name[21];


   notification *notif ;
   
 

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
            // TO-DO
            // ----------------- MUTEX -----------------------------
            handle_send(notif, message, profile_id, newsockfd);
            // ----------------- MUTEX -----------------------------

         break;

         case CMD_FOLLOW:
            strcpy(follow_name,message.payload);
            // TO-DO
            // ----------------- MUTEX -----------------------------
            handle_follow(follow_name, profile_id, newsockfd);  
            // ----------------- MUTEX ----------------------------- 
            print_pnd_notifs(profile_list[profile_id]);
         break;

         case INIT_USER:
            profile_id = handle_profile(message.payload,newsockfd);
         break;
      }
      
      free(message.payload);
   }

}

void *handle_client_consumes(void *arg) {

   int newsockfd = *(int *) arg;



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

      if(pthread_create(&client_pthread[i], NULL, handle_client_messages, &newsockfd) != 0 ){
         printf("Failed to create thread");
         exit(1);
      }

      if(pthread_create(&client_pthread[i], NULL, handle_client_consumes, &newsockfd) != 0 ){
         printf("Failed to create thread");
         exit(1);
      }
      
      i++;
   }
	  
   close(sockfd);
   printf("Server ended successfully\n");

   return 0;
}