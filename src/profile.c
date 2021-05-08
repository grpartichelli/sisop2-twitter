#include "../include/profile.h"
#include "../include/communication.h"

void primary_multicast(int userid,int type, int sqn, int len, int timestamp, char* payload);

//Add profile if it doesn't exist, else add to online 
int handle_profile(profile *profile_list, char *username, int newsockfd, int sqncnt, pthread_mutex_t online_mutex){
    char payload[150];
   //SEND TO THE OTHER RMs
  
               

   int profile_id = get_profile_id(profile_list,username);

   if(profile_id == -1){ //CASO NÃO EXISTA 

      //INSERE
      profile_id = insert_profile(profile_list, username,online_mutex);

      //AVISA OS RMS
      strcpy(payload,profile_list[profile_id].name);
      primary_multicast(profile_id ,CREATE_USER, ++sqncnt,strlen(payload)+1,getTime(),payload);
      primary_multicast(profile_id ,ADD_ONLINE, ++sqncnt,strlen(payload)+1,getTime(),payload);

      if(profile_id == -1){
         printf("MAX NUMBER OF PROFILES REACHED\n");
         exit(1);
      }
   } 
   else{//CASO USUARIO JÁ EXISTA
      if(profile_list[profile_id].online > 1){ //MAXIMO NUMERO DE ACESSOS É 2
         
         printf("Um usuario tentou exceder o numero de acessos.\n");
         send_packet(newsockfd,CMD_QUIT,sqncnt,4,0,"quit");
         close(newsockfd);
         return -1;
      }
      else{
         //AUMENTA A QUANTIDADE DE USUARIOS ONLINE
          pthread_mutex_lock(&online_mutex);
          profile_list[profile_id].online +=1;

          strcpy(payload,profile_list[profile_id].name);
          primary_multicast(profile_id ,ADD_ONLINE, ++sqncnt,strlen(payload)+1,getTime(),payload);
          pthread_mutex_unlock(&online_mutex);
         
      }
   }

   

   return profile_id;

}

void init_profiles(profile* profile_list){

	for(int i =0; i<MAX_CLIENTS; i++){
		profile_list[i].name= "";
		profile_list[i].online= 0;
    profile_list[i].port1= -1;
    profile_list[i].port2= -1;
	}
}

void print_profiles(profile* profile_list){

	for(int i =0; i<MAX_CLIENTS; i++){
		if(profile_list[i].name != ""){
			printf("Profile: %s Online %d\n", profile_list[i].name, profile_list[i].online);
		}
	}
}

int insert_profile(profile* profile_list, char* username,pthread_mutex_t online_mutex){

    for(int i =0; i<MAX_CLIENTS; i++){
        if(profile_list[i].name == ""){
        	
        	profile_list[i].name = (char*)malloc(strlen(username)+1);
            strcpy(profile_list[i].name,username);

            pthread_mutex_lock(&online_mutex);
            profile_list[i].online = 1;
            pthread_mutex_unlock(&online_mutex);

            profile_list[i].num_followers = 0;
            profile_list[i].num_snd_notifs = 0;
            profile_list[i].num_pnd_notifs = 0;

            //Initializing pending and send notifications
            for(int j=0;j<MAX_NOTIFS; j++){

            	profile_list[i].pnd_notifs[j].notif_id = -1;
            	profile_list[i].pnd_notifs[j].profile_id = -1;
            	profile_list[i].snd_notifs[j]= NULL;
            }


            return i;
        }
    }

    return -1;
}


int get_profile_id(profile* profile_list, char *username){

	for(int i =0; i<MAX_CLIENTS; i++){
		if(profile_list[i].name != ""){
			
			if(strcmp(profile_list[i].name,username)== 0){
				return i;
			}
		}
	}
	return -1;

}


void print_profile_pointers(profile** profile_pointers){
	profile p;
	for(int i =0; i<MAX_FOLLOW; i++){
		p = (*profile_pointers)[i];
		if(p.name != ""){
			printf("Profile: %s Online %d\n", p.name, p.online);
		}
	}
}

void print_pnd_notifs(profile p){
	int pid,nid;
	for(int i=0;i<p.num_pnd_notifs;i++){
         pid = p.pnd_notifs[i].profile_id;
         nid = p.pnd_notifs[i].notif_id;
         printf("Profile Id %d, Notif Id %d\n",pid,nid );
   }
}