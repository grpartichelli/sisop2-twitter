#include "../include/profile.h"



void init_profiles(profile* profile_list){


	for(int i =0; i<MAX_CLIENTS; i++){
		profile_list[i].name= "";
		profile_list[i].online= 0;
	}

}


void print_profiles(profile* profile_list){


	for(int i =0; i<MAX_CLIENTS; i++){
		printf("Profile: %s Online %d\n", profile_list[i].name, profile_list[i].online);
	}

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