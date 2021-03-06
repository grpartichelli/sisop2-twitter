#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#define MAX_NOTIFS 750

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct notification{
 uint32_t id;           // Notification identifier (int, unique)
 char* sender;          // @ of the user who sent the notif
 uint32_t timestamp;    // Timestamp
 char* msg;       // Message
 uint16_t len;          // Message length
 uint16_t pending;      // Number of pending readers
 } notification;


typedef struct notif_identifier{

int profile_id;  // Who sent the notification
int notif_id;  //The id of the notification

}notif_identifier;

void printNotif(notification notif);

#endif