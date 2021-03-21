#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>

#define BUFFER_SIZE 256

//Get client input and send to server
void *client_input(void *arg) {
    int n, sockfd  = *(int *) arg;

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);


    printf("Enter the message: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    
    /* write in the socket */
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0){
        printf("ERROR writing to socket\n");
        exit(1);
    } 


        
}

//Receive server response and display to user
void *client_display(void *arg) {
    int n, sockfd  = *(int *) arg;

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    
    /* read from the socket */
    n = read(sockfd, buffer, BUFFER_SIZE);
    while(n < 0){
        n = read(sockfd, buffer, BUFFER_SIZE);
    } 

    printf("%s\n",buffer);
 
}



int main(int argc, char *argv[])
{
    pthread_t thr_client_input, thr_client_display;

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    

    char profile[20];
    int port;

    //Check if correct input
    if (argc < 4) {
		fprintf(stderr,"Usage: ./app_cliente <profile> <server_address> <port>\n");
		exit(1);
    }

	////////////////////////////////
    //TODO Check for correct USER
    strcpy(profile,argv[1]);
    //////////////////////////////


    //Check for correct HOST
    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }
    /////////////////////////////////

    ////////////////////////////////
    //TODO Check for correct PORT
    port = atoi(argv[3]);
    //////////////////////////////

    // OPENING AND CONNECTING TO SOCKET
    printf("Profile: %s , Port: %d\n",profile,port);


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket\n");
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
	
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(1);
    }
    /////////////////////////////////////////////////////////

    pthread_create(&thr_client_input, NULL, client_input, &sockfd);
    pthread_create(&thr_client_display, NULL, client_display, &sockfd);
    
    pthread_join(thr_client_input, NULL);    
    pthread_join(thr_client_display, NULL);  
    
	close(sockfd);
    return 0;
}