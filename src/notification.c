#include "../include/notification.h"

void printNotif(notification notif){

	printf("[%.0i:%2.0i] %s - %s\n", notif.timestamp/100, notif.timestamp%100, notif.sender, notif.msg);

}