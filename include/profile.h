#ifndef PROFILE_H
#define PROFILE_H


#include "notification.h"
#define MAX_CLIENTS 500


typedef struct profile{
 char* id;              // Profile identifier (@(...))
 int online;            // Number of sessions open with this specific user
 char** followers;      // List of followers
 notification* rcv_notifs[MAX_NOTIFS]; // List of received notifs
 notification* pnd_notifs[MAX_NOTIFS]; // List of pending notifs
 } profile;


#endif