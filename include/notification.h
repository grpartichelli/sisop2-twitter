#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#define MAX_NOTIFS 500


typedef struct notification{
 uint32_t id;           // Notification identifier (int, unique)
 uint32_t timestamp;    // Timestamp
 const char* msg;       // Message
 uint16_t len;          // Message length
 uint16_t pending;      // Number of pending readers
 } notification;


typedef struct notif_identifier{

int profile_id;  // Who sent the notification
int notif_id;  //The id of the notification

}notif_identifier;

 #endif