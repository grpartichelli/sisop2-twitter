#ifndef PROFILE_H
#define PROFILE_H


#include "notification.h"
#define MAX_CLIENTS 750
#define MAX_FOLLOW 750

typedef struct profile{
 char* name;              // Profile identifier (@(...))
 int online;            // Number of sessions open with this specific user
 
 int num_followers; //Number of followers of this user
 struct profile* followers[MAX_FOLLOW];  // List of people that follow his profile

 int num_snd_notifs; //Number of sent notifs
 notification* snd_notifs[MAX_NOTIFS]; // List of send notifs
 
 int num_pnd_notifs; 
 notif_identifier pnd_notifs[MAX_NOTIFS]; // List of notification identifiers
 } profile;



void init_profiles(profile* profile_list); //Loads all the starting profiles
int insert_profile(profile* profile_list, char* username); //Inserts a new profile
int get_profile_id(profile* profile_list, char *username); //Gets a profile bid by name


void print_profile_pointers(profile** profile_pointers);
void print_pnd_notifs(profile p);
void print_profiles(profile* profile_list);


#endif