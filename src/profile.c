#include "../include/profile.h"



void init_profiles(profile* profile_list){

	for(int i =0; i<MAX_CLIENTS; i++){
		profile_list[i].name= "";
		profile_list[i].online= 0;
	}
}

void print_profiles(profile* profile_list){

	for(int i =0; i<MAX_CLIENTS; i++){
		if(profile_list[i].name != ""){
			printf("Profile: %s Online %d\n", profile_list[i].name, profile_list[i].online);
		}
	}
}

int insert_profile(profile* profile_list, char* username){

    for(int i =0; i<MAX_CLIENTS; i++){
        if(profile_list[i].name == ""){
        	profile_list[i].name = (char*)malloc(strlen(username)+1);
            strcpy(profile_list[i].name,username);
            profile_list[i].online = 1;
            profile_list[i].num_followers = 0;
            profile_list[i].num_snd_notifs = 0;
            profile_list[i].num_pnd_notifs = 0;
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