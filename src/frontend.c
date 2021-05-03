#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h> 
#include <signal.h>

#include "../include/frontend.h"

struct hostent *server;
void *primary_server_communication(void *arg);

int frontend_port = -1;
int primary_server_port = - 1;
int frontend_primary_socket;

void *frontend_run(void *arg){

	pthread_t thr_primary_server_communication;

	struct frontend_params *params = (struct frontend_params *) arg;
	server =  gethostbyname(params->host);
    primary_server_port = params->primary_port;

	struct sockaddr_in serv_addr,cli_addr;;
    int sockfd,yes=1, client_frontend_socket,clilen,found_port=0;
	int aux_port = 4999;//Default port
    
    //CREATE SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
    //OPEN SOCKET  
    print_error((sockfd < 0),"ERROR opening socket\n"); 

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    //FINDING AN AVAILABLE PORT
    while(!found_port){
    	aux_port++;
	    //Initializing structure
	   
	    serv_addr.sin_port = htons(aux_port);
	    
	    //BIND TO HOST
	    print_error((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1),"ERROR on setsockopt\n"); 
	  	 	
	  	//If the binding fails, we try again in another port.
	    if( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
	    	//If the binding works,set the port
	  		found_port = 1;
	    }
	}
	
	frontend_port = aux_port;


	//GET CLIENT_FRONTEND SOCKET
	//LISTEN
   	listen(sockfd,5);
   	clilen = sizeof(cli_addr);
   	//ACCEPT
	client_frontend_socket = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    print_error((client_frontend_socket < 0),"ERROR on accept\n");





  
    pthread_create(&thr_primary_server_communication, NULL, primary_server_communication, &client_frontend_socket);
    pthread_join(thr_primary_server_communication,NULL);

}

//Send the messages from the client to the server
void *primary_server_communication(void *arg){
	int client_frontend_socket = *(int *) arg;
	


	//GET FRONTEND_PRIMARY SOCKET
	struct sockaddr_in serv_addr,cli_addr;;
	print_error(((frontend_primary_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1), "ERROR opening socket\n"); 
    //CONNECT TO FRONTEND_PRIMARY SOCKET
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(primary_server_port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
   
    print_error((connect(frontend_primary_socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
	


	//Receive message
    packet message;
    while(1){

    	
   
    	receive(client_frontend_socket, &message);

    	if(message.type == CMD_QUIT){
    		exit(1);
	    }
    	
    	send_packet(frontend_primary_socket,message.type, message.sqn, message.len, message.timestamp,message.payload);
    	
    	


    	printf("%s\n", message.payload);

    	free(message.payload);
   	};
}

//return the frontend port to the user
int get_frontend_port(){

	//frontend might not have found the port yet, waiting for it to find
	while(frontend_port == -1);


	return frontend_port;
    
}
 