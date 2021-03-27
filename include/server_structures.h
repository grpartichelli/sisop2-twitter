// Gerenciador de comunicação: OK?
// Gerenciador de notificações: TO-DO
// Gerenciador de perfis e sessões: TO-DO
#define MAX_NOTIFS 500


typedef struct notification{
 uint32_t id;           // Notification identifier (int, unique)
 uint32_t timestamp;    // Timestamp
 const char* msg;       // Message
 uint16_t len;          // Message length
 uint16_t pending;      // Number of pending readers
 } notification;

typedef struct profile{
 char* id;              // Profile identifier (@(...))
 int online;            // Number of sessions open with this specific user
 char** followers;      // List of followers
 notification* rcv_notifs[MAX_NOTIFS]; // List of received notifs
 notification* pnd_notifs[MAX_NOTIFS]; // List of pending notifs
 } profile;