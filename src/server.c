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

#include "communication.c"
#include "notification.c"
#include "error_handler.c"
#include "file_handler.c"
#include "rm.c"


//configuration of this RM
rm this_rm;
//configuration of primary RM
rm primary_rm;
//configuration of other RM
rm rm_list[MAX_RMS];
int rm_list_size;


//Struct used for creating threads
typedef struct thread_parameters{ 
   int socket;
   int profile_id;
   int flag; 
}thread_parameters;

int sqncnt = 0;
int sockfd;

//THREADS
pthread_t client_pthread[MAX_CLIENTS*2]; //Each client has two threads, one for consuming and one for producing

//MUTEX
pthread_mutex_t send_mutex =  PTHREAD_MUTEX_INITIALIZER; //Making sure sends can't alter notifications at the same time
pthread_mutex_t follow_mutex = PTHREAD_MUTEX_INITIALIZER;//Making sure follow can't alter followers at the same time
pthread_mutex_t online_mutex = PTHREAD_MUTEX_INITIALIZER;//Not subtracting or adding a profile online info at the same time
//CONSUMER BARRIER
pthread_barrier_t  barriers[MAX_CLIENTS]; 

//LIST OF PROFILES
profile profile_list[MAX_CLIENTS];

//detecting ctrl+c
void intHandler(int dummy) {
   close(sockfd);   
   save_profiles(profile_list,this_rm.id);
   printf("\nServer ended successfully\n");
   exit(0);
}

int validate_follow(int follow_id,int profile_id, char *follow_name, int newsockfd){
   char payload[100];

   // CHECK FOR UNALLOWED FOLLOWERS
   if(follow_id == -1){ //User doesnt exist
      strcpy(payload,"FOLLOW falhou, usuario nao encontrado." );
      send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
      return 0;
   }

   
   //User trying to follow himself
   if(strcmp(follow_name,profile_list[profile_id].name) == 0 ){
      strcpy(payload,"FOLLOW falhou, voce nao pode se seguir.");
      send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
      return 0;
   }
   

   //User already follows
   for(int i=0;i<profile_list[follow_id].num_followers;i++){
      
      if(strcmp(profile_list[follow_id].followers[i]->name,profile_list[profile_id].name)==0){

         strcpy(payload,"FOLLOW falhou, voce nao pode seguir alguem duas vezes.");
         send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload); 
         return 0;
      }

   }
   return 1;
}


void handle_follow(char *follow_name, int profile_id, int newsockfd){

   char payload[100];  
   int follow_id;
   int num_followers;

   follow_id = get_profile_id(profile_list,follow_name);

   //check if follower is legit
   if(!validate_follow(follow_id,profile_id,follow_name,newsockfd))
      return;
  
   //CHECK IF USER CAN FOLLOW
   num_followers =  profile_list[follow_id].num_followers;
   print_error((num_followers >= MAX_FOLLOW),"Account reached max number of followers\n");
    
   //ADD FOLLOWER
   profile_list[follow_id].num_followers++;
   profile_list[follow_id].followers[num_followers] =  &profile_list[profile_id];

   //SEND TO USER THAT FOLLOW WAS SUCCESSFULL
   strcpy(payload,"FOLLOW executou com sucesso.");
   send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload);  
   
}

void handle_send(notification *notif, packet message, int profile_id, int newsockfd){
   char payload[100]; 
   
   profile *p;
   int num_pnd_notifs;
   int num_followers = profile_list[profile_id].num_followers;
   int notif_id;
   
   //Update notif id
   notif_id = profile_list[profile_id].num_snd_notifs;
   profile_list[profile_id].num_snd_notifs++;

   if(notif_id == MAX_NOTIFS){//Making it circular, will erase the first notification 
      notif_id = 0;           //if the server didnt send it (it should have by then)
   }

   //Create notification
   notif =  malloc(sizeof(notification));
   notif->id = notif_id;
   notif->sender = profile_list[profile_id].name;
   notif->timestamp = message.timestamp;
   notif->msg = (char*)malloc(strlen(message.payload)*sizeof(char)+sizeof(char));
   memcpy(notif->msg,message.payload,strlen(message.payload)*sizeof(char)+sizeof(char));
   notif->len = message.len;
   notif->pending = profile_list[profile_id].num_followers;

   //Putting the notification on the current profile as send
   profile_list[profile_id].snd_notifs[notif_id] = notif;

   
   //Putting the notification of followers pending list
   for(int i=0; i< num_followers;i++){

      p = profile_list[profile_id].followers[i];

      num_pnd_notifs = p->num_pnd_notifs; 
      p->num_pnd_notifs++;

      if(p->num_pnd_notifs == MAX_NOTIFS){//Making it circular, will erase the first notification 
         p->num_pnd_notifs =0;           //if the server didnt send it (it should have by then)
      }

      p->pnd_notifs[num_pnd_notifs].notif_id= notif_id;
      p->pnd_notifs[num_pnd_notifs].profile_id= profile_id;
   
   }

   //SEND TO USER SEND WAS SUCCESSFULL
   strcpy(payload,"SEND executou com sucesso.");
   send_packet(newsockfd,CMD_SEND,++sqncnt,strlen(payload)+1,0,payload);  



}
  

void *handle_client_messages(void *arg) {

   //Getting the parameters
   thread_parameters *par = (thread_parameters *) arg;
   int newsockfd = par->socket;
   int profile_id = par->profile_id;
   /////
   
   packet message;
   char follow_name[21];
   notification *notif ;
   
 

   signal(SIGINT, intHandler); //detect ctrl+c
   while(par->flag){
      
      //READ
      receive(newsockfd, &message);

      switch(message.type)
      {
         case CMD_QUIT:     
            send_packet(newsockfd,SRV_MSG,++sqncnt,1,0,"");
            
            
            pthread_mutex_lock(&online_mutex);
            profile_list[profile_id].online -=1;
            pthread_mutex_unlock(&online_mutex); 

            
            pthread_barrier_init (&barriers[profile_id], NULL, profile_list[profile_id].online);

           
            close(newsockfd);
            par->flag = 0;
         break;

         case CMD_SEND: 
            //Two threads cannot alter notifications at the same time
            pthread_mutex_lock(&send_mutex);



            handle_send(notif, message, profile_id, newsockfd); 



            pthread_mutex_unlock(&send_mutex); 
         break;

         case CMD_FOLLOW:
            //Two threads cannot alter the same follower at the same time
            pthread_mutex_lock(&follow_mutex); 



            strcpy(follow_name,message.payload);
            handle_follow(follow_name, profile_id, newsockfd); 



            pthread_mutex_unlock(&follow_mutex);  
         break;

         
      }
      
      free(message.payload);
   }

}

void *handle_client_consumes(void *arg) {
   
   //Getting the parameters
   thread_parameters *par = (thread_parameters *) arg;
   int newsockfd = par->socket;
   int profile_id = par->profile_id;

   notif_identifier notif_identifier;
   

   profile *p = &profile_list[profile_id];
   notification *n;
   char *str_notif; //String correspondent to the notification


   while(par->flag){//Flag synchronized with other thread
      //Iterating through pending notifs
      for(int i=0; i < p->num_pnd_notifs; i++){
         

         notif_identifier = p->pnd_notifs[i];
         if(notif_identifier.profile_id != -1){ //Notification exists
            
            //Get the current notification
            n = profile_list[notif_identifier.profile_id].snd_notifs[notif_identifier.notif_id];
            str_notif=malloc(n->len+strlen(n->sender)+12*sizeof(char));
            sprintf(str_notif,"[%.0i:%02d] %s - %s\n", n->timestamp/100, n->timestamp%100, n->sender, n->msg);

            //Send the notification
            send_packet(newsockfd,NOTIF,++sqncnt,strlen(str_notif), n->timestamp, str_notif);
            free(str_notif);
             
            //BARRIER FOR CONCURRENT CLIENTS
            pthread_barrier_wait (&barriers[profile_id]);

            //START MUTEX
            //Not allowing notifications to be changed at the same time
            pthread_mutex_lock(&send_mutex); 
            
            if(par->flag){
               
               if(p->pnd_notifs[i].profile_id != -1){ //Test if it hasnt been deleted (multiple of the same clients case)
                  
                  n->pending--; //Subtract number of pending readers    
                  if(n->pending == 0){ 
                     //Delete notification from sender
                     n = NULL;
                  }

                  //Delete notification identifier from client
                  p->pnd_notifs[i].profile_id = -1;
                  p->pnd_notifs[i].notif_id = -1;

               }
            }
            //END MUTEX
            pthread_mutex_unlock(&send_mutex);
           
         }
         
      }

   }


   
}

void init_barriers(){

   for(int i=0;i<MAX_CLIENTS;i++){
       pthread_barrier_init (&barriers[i], NULL, 0);
   }
}



int main( int argc, char *argv[] ) {

   //Check if correct input
   print_error((argc < 3),"Usage: ./server <config_file.txt> <id>\n");
  
   //Read RM scenario config
   read_config_file(argv[1], atoi(argv[2]), &this_rm, &primary_rm, rm_list, &rm_list_size);
   printf("ID: %d, PORT: %d ", this_rm.id, this_rm.port);
   if(this_rm.is_primary){printf("TYPE: PRIMARY\n");} else{printf("TYPE: BACKUP\n");}


   int i=0,profile_id;
   int newsockfd, portno, clilen;
   int yes =1;
   struct sockaddr_in serv_addr, cli_addr;
   packet message;
   thread_parameters parameters[MAX_CLIENTS];
   
   //INIT STRUCTURES
   init_profiles(profile_list);
   read_profiles(profile_list,this_rm.id);
   init_barriers();
  

   //CREATE SOCKET
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   //OPEN SOCKET  
   print_error((sockfd < 0),"ERROR opening socket\n");  
  

   //Initializing structure
   bzero((char *) &serv_addr, sizeof(serv_addr)); 
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(this_rm.port);

   //BIND TO HOST
   print_error((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1),"ERROR on setsockopt\n"); 
   print_error((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0),"ERROR on binding.\n"); 

   printf("Server started correctly.\n");

	//LISTEN
   listen(sockfd,5);
   clilen = sizeof(cli_addr);


   while(1){
      signal(SIGINT, intHandler); //detect ctrl+c
      //ACCEPT
      newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
      print_error((newsockfd < 0),"ERROR on accept");

      //Receive message
      receive(newsockfd, &message);
      print_error((message.type != INIT_USER),"Error, user not initialized.\n");
              
      //Create or update profile
      profile_id = handle_profile(profile_list, message.payload,newsockfd, ++sqncnt,online_mutex);
      free(message.payload);

      if(profile_id != -1){
         //Update barrier in case number of online user changed
         pthread_barrier_init (&barriers[profile_id], NULL, profile_list[profile_id].online);
         

         //LOAD PARAMETERS FOR THREADS
         parameters[i].profile_id = profile_id;
         parameters[i].socket = newsockfd;
         parameters[i].flag = 1;
         
         //One thread consumes notifications, the other reads user input
         print_error((pthread_create(&client_pthread[i], NULL, handle_client_messages, &parameters[i]) != 0 ), "Failed to create handle client messages thread\n");
         print_error((pthread_create(&client_pthread[i+1], NULL, handle_client_consumes, &parameters[i]) != 0 ),"Failed to create consume thread.\n" );
         i+=2;
      }
      
      
   }
	  
   close(sockfd);
   printf("Server ended successfully\n");

   return 0;
}