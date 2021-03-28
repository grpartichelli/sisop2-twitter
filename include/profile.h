#ifndef PROFILE_H
#define PROFILE_H


#include "notification.h"
#define MAX_CLIENTS 750
#define MAX_FOLLOW 750

typedef struct profile{
 char* name;              // Profile identifier (@(...))
 int online;            // Number of sessions open with this specific user
 int num_followers; //Number of followers of this user
 struct profile* followers[MAX_FOLLOW];      // List of people that follow his profile
 notification* rcv_notifs[MAX_NOTIFS]; // List of received notifs
 notification* pnd_notifs[MAX_NOTIFS]; // List of pending notifs
 } profile;


#endif