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
#include <netdb.h> 
#include <signal.h>

#include "communication.c"
#include "notification.c"
#include "error_handler.c"
#include "file_handler.c"
#include "rm.c"

#define TIMEOUT 1
#define HEART_TIME 1

/////////////////////////////////////////
//ETAPA2
//configuration of this RM
rm this_rm;
//configuration of primary RM
rm primary_rm;
//configuration of other RM
rm rm_list[MAX_RMS];
int rm_list_size;
int get_rm_list_index(int id);
void backup_connect_to_primary();
void backup_connect_to_backup(int id);
void *backup_accept_new_backups(void *arg);
void *send_heartbeat(void *arg);
void backup_create_user(packet message, int new_user);
void backup_send_ack_to_primary();
void backup_add_follower(int follower, int followed);
void backup_handle_send(packet message, int profile_id);
void backup_handle_consume(int profile_id, int notif_id);
void primary_multicast(int userid,int type, int sqn, int len, int timestamp, char* payload);
void bully_election();
void* receive_bully_messages(void *arg);


//////////////////////////////////////////
//Struct used for creating threads
typedef struct thread_parameters{ 
   int socket;
   int profile_id;
   int flag; 
}thread_parameters;

int sqncnt = 0,clilen;
struct sockaddr_in serv_addr, cli_addr;

//THREADS
pthread_t client_pthread[MAX_CLIENTS*2]; //Each client has two threads, one for consuming and one for producing
pthread_t thr_backup_accept_backups;
pthread_t thr_send_heartbeat;
pthread_t thr_bully_receive[MAX_RMS];

//MUTEX
pthread_mutex_t send_mutex =  PTHREAD_MUTEX_INITIALIZER; //Making sure sends can't alter notifications at the same time
pthread_mutex_t follow_mutex = PTHREAD_MUTEX_INITIALIZER;//Making sure follow can't alter followers at the same time
pthread_mutex_t online_mutex = PTHREAD_MUTEX_INITIALIZER;//Not subtracting or adding a profile online info at the same time
pthread_mutex_t ack_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_update_mutex = PTHREAD_MUTEX_INITIALIZER;//Not subtracting or adding a profile online info at the same time
//CONSUMER BARRIER
pthread_barrier_t  barriers[MAX_CLIENTS]; 

//LIST OF PROFILES
profile profile_list[MAX_CLIENTS];

//VARIABLES FOR BULLY
int thread_open[MAX_RMS];
int coordinator_received = 0;
int answer_cnt = 0;
int election_started = 0;
int election_received = 0;

//detecting ctrl+c
void intHandler(int dummy) {
   close(this_rm.socket);   
   save_profiles(profile_list,this_rm.id);
   printf("\nServer ended successfully\n");
   exit(0);
}

int validate_follow(int follow_id,int profile_id, char *follow_name, int newsockfd){
   char payload[100];

   // CHECK FOR UNALLOWED FOLLOWERS
   if(follow_id == -1){ //User doesnt exist
      strcpy(payload,"FOLLOW falhou, usuario nao encontrado." );
      if (send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload))
         return 0;
   }

   
   //User trying to follow himself
   if(strcmp(follow_name,profile_list[profile_id].name) == 0 ){
      strcpy(payload,"FOLLOW falhou, voce nao pode se seguir.");
      if(send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload))
         return 0;
   }
   

   //User already follows
   for(int i=0;i<profile_list[follow_id].num_followers;i++){
      
      if(strcmp(profile_list[follow_id].followers[i]->name,profile_list[profile_id].name)==0){

         strcpy(payload,"FOLLOW falhou, voce nao pode seguir alguem duas vezes.");
         if(send_packet(newsockfd,CMD_FOLLOW,++sqncnt,strlen(payload)+1,0,payload))
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

   //UPDATE THE RMS
   primary_multicast( follow_id,CMD_FOLLOW, ++sqncnt,strlen(profile_list[profile_id].name)+1,getTime(),profile_list[profile_id].name);
   

   //SAVE PROFILES
   save_profiles(profile_list,this_rm.id);

   //SEND TO USER THAT FOLLOW WAS SUCCESSFULL
   strcpy(payload,"FOLLOW executou com sucesso.");
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



   //UPDATE THE RMS
   primary_multicast(profile_id, CMD_SEND, message.sqn, strlen(message.payload)+1,message.timestamp,message.payload);
   

   //SAVE PROFILES
   save_profiles(profile_list,this_rm.id);

   //SEND TO USER SEND WAS SUCCESSFULL
   char payload[100];
   strcpy(payload,"SEND executou com sucesso.");
   send_packet(newsockfd,CMD_SEND,++sqncnt,strlen(payload)+1,0,payload);  



}
  

void *handle_client_messages(void *arg) {
   char payload[150];
   //Getting the parameters
   thread_parameters *par = (thread_parameters *) arg;
   int newsockfd = par->socket;
   int profile_id = par->profile_id;
   int port;
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
            if(send_packet(newsockfd,CMD_QUIT,++sqncnt,1,0,"")){
           
               pthread_mutex_lock(&online_mutex);
               profile_list[profile_id].online -=1;
            
               //SENDO RMS
               strcpy(payload,profile_list[profile_id].name);
               primary_multicast(profile_id ,SUB_ONLINE, ++sqncnt,strlen(payload)+1,getTime(),payload);
               pthread_mutex_unlock(&online_mutex); 

            
               pthread_barrier_init (&barriers[profile_id], NULL, profile_list[profile_id].online);

               
               
               pthread_mutex_lock(&socket_update_mutex);
               if(profile_list[profile_id].port1 == port){
                  profile_list[profile_id].port1 = -1;
                  primary_multicast(profile_id ,REMOVE_PORT1, ++sqncnt,1,getTime(),"");
                 
               }
               else{
                  profile_list[profile_id].port2 = -1;
                  primary_multicast(profile_id ,REMOVE_PORT2, ++sqncnt,1,getTime(),"");
                  
               }
               pthread_mutex_unlock(&socket_update_mutex);  
               
               par->flag = 0;
            }
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

          case UPDATE_PORT:
          
            //Frontend port, used so the new backup can connect to this client
            pthread_mutex_lock(&socket_update_mutex);
            port = atoi(message.payload);
            
            if(profile_list[profile_id].port1 == -1){
               profile_list[profile_id].port1 = port;
               primary_multicast(profile_id ,UPDATE_PORT1, ++sqncnt,strlen(message.payload)+1,getTime(),message.payload);
            }
            
            else{
               profile_list[profile_id].port2 = port;
               primary_multicast(profile_id ,UPDATE_PORT2, ++sqncnt,strlen(message.payload)+1,getTime(),message.payload);
           
            }

            pthread_mutex_unlock(&socket_update_mutex);  
            
         break;

         
      }
      
      
      if(message.payload){
         free(message.payload);
      } 
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
            if(str_notif){
               free(str_notif);
            } 
             
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

                  //SEND THE CHANGE TO THE RMS
                  char str_i[10];
                  sprintf(str_i, "%d", i);
                  primary_multicast(profile_id ,NOTIF_CONSUMED, ++sqncnt,strlen(str_i)+1,getTime(),str_i);

               }
            }

             //SAVE PROFILES
             save_profiles(profile_list,this_rm.id);

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

//Wait for an acknowladge from rm_id
void primary_receive_ack(int rm_id){
   packet message;
   if(!receive(rm_list[rm_id].socket,&message))
   {  
      rm_list[rm_id].socket = -1;
   }
   else

   {  
      if(message.type != ACK){
         exit(1);
      }
       if(message.payload){
         free(message.payload);
       } 

   }      
}

//send a packet to every socket connect to the primary
void primary_multicast(int userid,int type, int sqn, int len, int timestamp, char* payload){

   pthread_mutex_lock(&ack_mutex);
  
   for(int i=0;i<rm_list_size;i++){
      if(rm_list[i].socket != -1){
         if (send_packet_with_userid(rm_list[i].socket,userid,type, sqn,len,timestamp,payload)){
            primary_receive_ack(i);
         }
         else{
            rm_list[i].socket = -1;
         }
      }
   }
   pthread_mutex_unlock(&ack_mutex);
}

//sends the initial info needed for a backup to be synchronized
void primary_send_initial_info(int rm_index){
 
   char payload[150];
   
   //Send every user that exists
   for(int i =0; i<MAX_CLIENTS; i++){
      
      if(profile_list[i].name != "" && profile_list[i].name[0] == '@' && rm_list[rm_index].socket!=-1){ 
         strcpy(payload,profile_list[i].name);
         if(send_packet_with_userid(rm_list[rm_index].socket, i ,CREATE_USER, ++sqncnt,strlen(payload)+1,getTime(),payload))
            primary_receive_ack(rm_index);  
         else
            rm_list[rm_index].socket = -1;

         for(int k =0; k<profile_list[i].online;k++){
            if(send_packet_with_userid(rm_list[rm_index].socket,i,ADD_ONLINE, ++sqncnt,strlen(payload)+1,getTime(),payload))
               primary_receive_ack(rm_index); 
            else
               rm_list[rm_index].socket = -1;
         }
      }
      else{
         break;
      }
   }

   for(int i =0; i<MAX_CLIENTS; i++){
      if(profile_list[i].name != "" && profile_list[i].name[0] == '@'){ 
         //Send who follows the user
         for(int j=0; j<profile_list[i].num_followers;j++){
            strcpy(payload,profile_list[i].followers[j]->name);
            if(send_packet_with_userid(rm_list[rm_index].socket, i ,CMD_FOLLOW, ++sqncnt,strlen(payload)+1,getTime(),payload))
               primary_receive_ack(rm_index);
            else
               rm_list[rm_index].socket = -1;
         }


         sprintf(payload, "%d", profile_list[i].port1);
         if(send_packet_with_userid(rm_list[rm_index].socket, i ,UPDATE_PORT1, ++sqncnt,strlen(payload)+1,getTime(),payload))
               primary_receive_ack(rm_index);
            else
               rm_list[rm_index].socket = -1;

         sprintf(payload, "%d", profile_list[i].port2);
         if(send_packet_with_userid(rm_list[rm_index].socket, i ,UPDATE_PORT2, ++sqncnt,strlen(payload)+1,getTime(),payload))
               primary_receive_ack(rm_index);
            else
               rm_list[rm_index].socket = -1;

      }
      else{
         return;
      }
   }
          
}


int main( int argc, char *argv[] ) {

   //Check if correct input
   print_error((argc < 3),"Usage: ./server <config_file.txt> <id>\n");
  
   //Read RM scenario config
   read_config_file(argv[1], atoi(argv[2]), &this_rm, &primary_rm, rm_list, &rm_list_size);
   printf("ID: %d, PORT: %d ", this_rm.id, this_rm.port);
   if(this_rm.is_primary){printf("TYPE: PRIMARY\n");} else{printf("TYPE: BACKUP\n");}

   struct hostent *server;
   int i=0,profile_id, rm_list_index;
   int newsockfd, portno;
   int yes =1;
   char payload[150];
   
   packet message;
   thread_parameters parameters[MAX_CLIENTS];
   
   //INIT STRUCTURES
   init_profiles(profile_list);
   read_profiles(profile_list,this_rm.id);
   init_barriers();
  

   //CREATE SOCKET
   this_rm.socket = socket(AF_INET, SOCK_STREAM, 0);
   
   //OPEN SOCKET  
   print_error((this_rm.socket < 0),"ERROR opening socket\n");  
  

   //Initializing structure
   bzero((char *) &serv_addr, sizeof(serv_addr)); 
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(this_rm.port);

   //BIND TO HOST
   print_error((setsockopt(this_rm.socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1),"ERROR on setsockopt\n"); 
   print_error((bind(this_rm.socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0),"ERROR on binding.\n"); 

   printf("Server started correctly.\n");

	//LISTEN
   listen(this_rm.socket,5);
   clilen = sizeof(cli_addr);

 
    

   while(1){
      signal(SIGINT, intHandler); //detect ctrl+c


      //PRIMARY CODE
      if(this_rm.is_primary){
         print_error((pthread_create(&thr_send_heartbeat, NULL, send_heartbeat, NULL) != 0 ),"Failed to create accept backups thread.\n" );

         server = gethostbyname("localhost");         
         serv_addr.sin_family = AF_INET;     
         serv_addr.sin_port = htons(primary_rm.port);    
         serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
         bzero(&(serv_addr.sin_zero), 8); 
      
         for(int p_id =0; p_id<MAX_CLIENTS;p_id++){


        
               if(profile_list[p_id].port1 != -1){    
                  int new_socket = socket(AF_INET, SOCK_STREAM, 0);
                  serv_addr.sin_port = htons(profile_list[p_id].port1);    
                  print_error((connect(new_socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
                   
                  //Update barrier in case number of online user changed
                  pthread_barrier_init (&barriers[p_id], NULL, profile_list[p_id].online);
                     
                  //LOAD PARAMETERS FOR THREADS
                  parameters[i].profile_id = p_id;
                  parameters[i].socket = new_socket;
                  parameters[i].flag = 1;
                     
                  //One thread consumes notifications, the other reads user input
                  print_error((pthread_create(&client_pthread[i], NULL, handle_client_messages, &parameters[i]) != 0 ), "Failed to create handle client messages thread\n");
                  print_error((pthread_create(&client_pthread[i+1], NULL, handle_client_consumes, &parameters[i]) != 0 ),"Failed to create consume thread.\n" );
                  i+=2;
                  
                 
               }

               if(profile_list[p_id].port2 != -1){    
                  int new_socket = socket(AF_INET, SOCK_STREAM, 0);
                  serv_addr.sin_port = htons(profile_list[p_id].port2 );    
                  print_error((connect(new_socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
                   
                  //Update barrier in case number of online user changed
                  pthread_barrier_init (&barriers[p_id], NULL, profile_list[p_id].online);
                     
                  //LOAD PARAMETERS FOR THREADS
                  parameters[i].profile_id = p_id;
                  parameters[i].socket = new_socket;
                  parameters[i].flag = 1;
                     
                  //One thread consumes notifications, the other reads user input
                  print_error((pthread_create(&client_pthread[i], NULL, handle_client_messages, &parameters[i]) != 0 ), "Failed to create handle client messages thread\n");
                  print_error((pthread_create(&client_pthread[i+1], NULL, handle_client_consumes, &parameters[i]) != 0 ),"Failed to create consume thread.\n" );
                  i+=2;
                          
               }

            }
         
       
          while(1){    
            //ACCEPT
             newsockfd = accept(this_rm.socket, (struct sockaddr *)&cli_addr, &clilen);
            print_error((newsockfd < 0),"ERROR on accept");
          
            //Receive message
            receive(newsockfd, &message);
            switch(message.type){
               case INIT_USER:
                   //Create or update profile
                   profile_id = handle_profile(profile_list, message.payload,newsockfd, ++sqncnt,online_mutex);
                   
                   if(message.payload){
                     free(message.payload);
                   }

                   //SAVE PROFILES
                   save_profiles(profile_list,this_rm.id);

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
               break;


               case INIT_BACKUP:
                  rm_list_index = get_rm_list_index(atoi(message.payload)); 

                  if(message.payload){
                     free(message.payload); 
                  }
                
                  
                  rm_list[rm_list_index].socket = newsockfd;
                  //warn all the other backups this one connected
                  
                  primary_multicast(-1,INIT_BACKUP,++sqncnt,strlen(rm_list[rm_list_index].string_id)+1,getTime(),rm_list[rm_list_index].string_id);
                  
                  
                  //send all initial info to this backup
                  primary_send_initial_info(rm_list_index);

               break;   
            }

         }

      /////////////////////////////////////
      } else{//BACKUP CODE
         

         backup_connect_to_primary();

         //create thread that will accept new backups
         print_error((pthread_create(&thr_backup_accept_backups, NULL, backup_accept_new_backups, NULL) != 0 ),"Failed to create accept backups thread.\n" );
         
         int follower,followed;    

         int c;
         for(c = 0; c<MAX_RMS; c++)
         {
            thread_open[c] = 0;
         }

         while(!this_rm.is_primary){


            for(c = 0; c<rm_list_size; c++)
            {  
               if(!thread_open[rm_list[c].id] && rm_list[c].socket != -1 && !rm_list[c].is_primary )
               {
                  thread_open[rm_list[c].id] = 1;
                  print_error(pthread_create(&thr_bully_receive[rm_list[c].id], NULL, receive_bully_messages, (void*)((long int)&rm_list[c].id)),"Failed to create receive bully messages thread.");
               }
            }

            if(!receive(primary_rm.socket, &message))
            {  
               if(!election_started && !election_received)
               {
                  bully_election();
               }
            }else{
            
               switch(message.type){
                  case INIT_BACKUP:
                     //Start connection with this backup
                     if(atoi(message.payload) != this_rm.id){
                        backup_connect_to_backup(atoi(message.payload));    
                     }
                     
                  break;
                  case CREATE_USER:  
                     backup_create_user(message, message.userid);
                     printf("%s has been created.\n",message.payload);
                     
                  break;

                  case CMD_FOLLOW:
                     follower = get_profile_id(profile_list,message.payload );
                     backup_add_follower(follower ,message.userid);
                     printf("%s is now following %s.\n",message.payload,profile_list[message.userid].name);
                  break; 
                  case CMD_SEND:
                     backup_handle_send(message, message.userid);
                     printf("%s has sent the message: %s\n",profile_list[message.userid].name, message.payload);
                  break;

                  case NOTIF_CONSUMED:
                     backup_handle_consume(message.userid,atoi(message.payload));
                     printf("A notification was consumed by %s.\n",profile_list[message.userid].name);
                  break;

                  case ADD_ONLINE:
                     profile_list[message.userid].online++;
                     printf("Added 1 to %s online counter.\n",profile_list[message.userid].name);
                  break;

                  case SUB_ONLINE:
                     profile_list[message.userid].online--;
                     printf("Subtracted 1 from %s online counter.\n",profile_list[message.userid].name);
                  break;
                  
                  case REMOVE_PORT1:
                     profile_list[message.userid].port1 = -1;
                  break;
                  
                  case REMOVE_PORT2:
                     profile_list[message.userid].port2 = -1;        
                  break;
                  
                  case UPDATE_PORT1:
                     profile_list[message.userid].port1 = atoi(message.payload);      
                  break;
                  
                  case UPDATE_PORT2:
                     profile_list[message.userid].port2 = atoi(message.payload);
                  break;
                  
                  case HEARTBEAT:
                  break;

                }

                backup_send_ack_to_primary();
               

               save_profiles(profile_list,this_rm.id);
               if(message.payload){
                  free(message.payload);
               } 
            }  
         }
      }   
   }
	
   for(i = 0; i<MAX_CLIENTS; i++)
   {
      if(profile_list[i].name)
         free(profile_list[i].name);
   }
   close(this_rm.socket);
   printf("Server ended successfully\n");

   return 0;
}

/////////////////////////////////////////////
//BULLY 


void* receive_bully_messages(void *arg)
{
   int* id = (int*)arg;
   int index = get_rm_list_index(*id);
   packet message;
   int loop = 1;
   while(loop)
   {
      if(rm_list[index].socket!=-1)
      {
         int received = receive(rm_list[index].socket,&message);
         if(received)
         {
            switch(message.type)
            {
               case BULLY_COORDINATOR:
                  coordinator_received = message.userid;
                  if(thread_open[coordinator_received]){
                     pthread_cancel(thr_bully_receive[coordinator_received]);
                     thread_open[coordinator_received] = -1;
                  }  
                  loop = 0;
                  break;

               case BULLY_ANSWER:
                  answer_cnt++;
                  break;

               case BULLY_ELECTION:
                  send_packet_with_userid(rm_list[index].socket,this_rm.id,BULLY_ANSWER, ++sqncnt,strlen("answer")+1,getTime(),"answer");
                  if(!election_started && !election_received)
                     bully_election();
                  printf("Answer message sent to backup %i.\n", message.userid);
                  election_received = 1;
               break;

               default:
               break;
            }
            if(message.payload){
               free(message.payload);
            } 
         }
         else 
         {
            printf("No message was received from backup %i.\n", *id);
            rm_list[index].socket = -1;
         }
      }
   }
}

void update_primary(int coordinator_id)
{
   int id = get_rm_list_index(coordinator_id);

   primary_rm.id  = rm_list[id].id;
   primary_rm.socket = rm_list[id].socket;
   strcpy(primary_rm.string_id,rm_list[id].string_id);
   primary_rm.port  = rm_list[id].port;
   rm_list[id].socket = -1;

   printf("The new primary is %i.\n", coordinator_id);
}

void close_backup_threads()
{
   this_rm.is_primary = 1;
   pthread_cancel(thr_backup_accept_backups);
   
   for(int i=0;i<MAX_RMS;i++){
      if(thread_open[i] && thread_open[i]!=-1){
         pthread_cancel(thr_bully_receive[i]);
         thread_open[i] = -1;
      }  
   }
}

void become_primary()
{
   int i = 0;
   for(i = 0; i < rm_list_size; i++)
   {
      if(rm_list[i].socket != -1 && rm_list[i].id != this_rm.id)
      {
         if(!send_packet_with_userid(rm_list[i].socket,this_rm.id,BULLY_COORDINATOR, ++sqncnt,strlen("coord")+1,getTime(),"coord"))
            rm_list[i].socket = -1;
         else
         {  
            printf("Sent a coordinator message to %i\n", rm_list[i].id);
         }
      }               
   }
   close_backup_threads();  
}

void bully_election()
{
   printf("Starting bully election.\n");
   int i = 0;
   election_started = 1;
   answer_cnt = 0;
   coordinator_received = 0;
   for(i = 0; i<rm_list_size; i++)
   {
      if(rm_list[i].id > this_rm.id && rm_list[i].id != this_rm.id)
      {
         if(!send_packet_with_userid(rm_list[i].socket,this_rm.id,BULLY_ELECTION,++sqncnt,strlen("election")+1,getTime(),"election"))
         {
            rm_list[i].socket = -1;
         }
         else
            printf("Sent an election message to backup %i.\n", rm_list[i].id);
      }
   }

   sleep(TIMEOUT);

   if(!answer_cnt)
   {
      printf("I am the new primary.\n");
      become_primary();
   }
   else
   {
      sleep(TIMEOUT);
      if(coordinator_received)
      {
         update_primary(coordinator_received);
      }
      else 
      {
         printf("I am the new primary.\n");
         become_primary();
      }
   }    

   election_started = 0;
   election_received = 0;
   coordinator_received = 0;

}





//HANDLE FOLLOW
void backup_add_follower(int follower, int followed){

   //ADD FOLLOWER
   int num_followers =  profile_list[followed].num_followers;
   profile_list[followed].followers[num_followers] =  &profile_list[follower];
   profile_list[followed].num_followers++;

}

//BACKUP CODE
void backup_send_ack_to_primary(){
   if(!send_packet(primary_rm.socket,ACK, ++sqncnt,strlen("ack")+1,getTime(),"ack"))
   {
      primary_rm.socket = -1;
   }
}



//this is just for initial configuration of making the rm equal
void backup_create_user(packet message, int new_user){

   profile_list[new_user].name = (char*)malloc(strlen(message.payload)+1);
   strcpy(profile_list[new_user].name,message.payload);

   profile_list[new_user].num_followers = 0;
   profile_list[new_user].num_snd_notifs = 0;
   profile_list[new_user].num_pnd_notifs = 0;

    //Initializing pending and send notifications
   for(int j=0;j<MAX_NOTIFS; j++){
      profile_list[new_user].pnd_notifs[j].notif_id = -1;
      profile_list[new_user].pnd_notifs[j].profile_id = -1;
      profile_list[new_user].snd_notifs[j]= NULL;
   }


}

void *send_heartbeat(void *arg)
{
   packet message;

   while(1)
   {
      sleep(HEART_TIME);
      if(!election_started)
      {
         primary_multicast(-1, HEARTBEAT, ++sqncnt, strlen("I'm alive")+1, getTime(), "I'm alive");
         printf("Sending heartbeat!\n");
      }
   }
}

//thread that accepts new backups connections, so that all rms are socket connected
void *backup_accept_new_backups(void *arg){
   packet message;

   while(1){
      int tempsockfd = accept(this_rm.socket, (struct sockaddr *)&cli_addr, &clilen);
      print_error((tempsockfd < 0),"");

      //Receive message
      if(receive(tempsockfd, &message))
      {
         int rm_list_index = get_rm_list_index(atoi(message.payload)); 
         if(message.payload){
            free(message.payload);  
         } 
         rm_list[rm_list_index].socket = tempsockfd;
      }

      tempsockfd = -1;
   }
  
}


void backup_handle_consume(int profile_id, int notif_id){


   profile *p = &profile_list[profile_id];
   notif_identifier notif_identifier = p->pnd_notifs[notif_id];
   notification *n = profile_list[notif_identifier.profile_id].snd_notifs[notif_identifier.notif_id];

   if(p->pnd_notifs[notif_id].profile_id != -1){ //Test if it hasnt been deleted (multiple of the same clients case)
                  
         n->pending--; //Subtract number of pending readers    
         if(n->pending == 0){ 
            //Delete notification from sender
            n = NULL;
         }

         //Delete notification identifier from client
         p->pnd_notifs[notif_id].profile_id = -1;
         p->pnd_notifs[notif_id].notif_id = -1;

      }
}


void backup_handle_send(packet message, int profile_id){

   notification *notif;
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
}


//connect to a new backup by its id
void backup_connect_to_backup(int id){
   int rm_list_index = get_rm_list_index(id);

   struct sockaddr_in serv_addr,cli_addr;
   struct hostent *server;
   server = gethostbyname("localhost");
   //GET FRONTEND_PRIMARY SOCKET
   rm_list[rm_list_index].socket = socket(AF_INET, SOCK_STREAM, 0);
   if(rm_list[rm_list_index].socket == -1 && DEBUG) printf("ERROR opening socket\n");

   //CONNECT TO FRONTEND_PRIMARY SOCKET
   serv_addr.sin_family = AF_INET;     
   serv_addr.sin_port = htons(rm_list[rm_list_index].port);    
   serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
   bzero(&(serv_addr.sin_zero), 8);     
   

   print_error((connect(rm_list[rm_list_index].socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
   if(!send_packet(rm_list[rm_list_index].socket, INIT_BACKUP, ++sqncnt,strlen(this_rm.string_id)+1 , getTime(), this_rm.string_id))
      rm_list[rm_list_index].socket = -1;
   
}
//connect to the primary server socket
void backup_connect_to_primary(){
   struct sockaddr_in serv_addr,cli_addr;
   struct hostent *server;
   server = gethostbyname("localhost");
   //GET FRONTEND_PRIMARY SOCKET
   print_error(((primary_rm.socket = socket(AF_INET, SOCK_STREAM, 0)) == -1), "ERROR opening socket\n"); 
   //CONNECT TO FRONTEND_PRIMARY SOCKET
   serv_addr.sin_family = AF_INET;     
   serv_addr.sin_port = htons(primary_rm.port);    
   serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
   bzero(&(serv_addr.sin_zero), 8);     
   
   print_error((connect(primary_rm.socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 

   if(!send_packet(primary_rm.socket, INIT_BACKUP, ++sqncnt,strlen(this_rm.string_id)+1 , getTime(), this_rm.string_id))
      primary_rm.socket = -1;
}


//connect to the primary server socket
int get_rm_list_index(int id){

   for(int i =0; i<MAX_RMS; i++){
      if(rm_list[i].id == id){
         return i;
      }
   }

   return -1;
}
