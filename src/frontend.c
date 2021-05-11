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
#include <time.h>

#include "../include/frontend.h"
int get_frontend_port();

struct hostent *server;
void *client_to_primary_server(void *arg);
void *primary_server_to_client(void *arg);
void *receive_new_primary(void *arg);

int frontend_port = -1;
int primary_server_port = - 1;
int frontend_primary_socket;
int client_frontend_socket;


int sockfds = 0;
struct sockaddr_in serv_addr,cli_addr;
int clilen;

pthread_t thr_primary_server_to_client;
pthread_t thr_client_to_primary_server;

void *frontend_run(void *arg){

	
	pthread_t thr_receive_new_primary;

	struct frontend_params *params = (struct frontend_params *) arg;
	server =  gethostbyname(params->host);
    primary_server_port = params->primary_port;


    
	
    int yes=1,found_port=0;
	srand(time(0));
	int aux_port = (rand() % (5000 - 4050 + 1)) + 4050;//Default port
    
    //CREATE SOCKET
    sockfds = socket(AF_INET, SOCK_STREAM, 0);
   
    //OPEN SOCKET  
    print_error((sockfds < 0),"ERROR opening socket\n"); 

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    //FINDING AN AVAILABLE PORT
    while(!found_port){
    	aux_port = (rand() % (5000 - 4050 + 1)) + 4050;
	    //Initializing structure
	   
	    serv_addr.sin_port = htons(aux_port);
	    
	    //BIND TO HOST
	    print_error((setsockopt(sockfds, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1),"ERROR on setsockopt\n"); 
	  	 	
	  	//If the binding fails, we try again in another port.
	    if( bind(sockfds, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
	    	//If the binding works,set the port
	  		found_port = 1;
	    }
	}
	
	frontend_port = aux_port;


	//GET CLIENT_FRONTEND SOCKET
	//LISTEN
   	listen(sockfds,5);
   	clilen = sizeof(cli_addr);
   	//ACCEPT
	client_frontend_socket = accept(sockfds, (struct sockaddr *)&cli_addr, &clilen);
    print_error((client_frontend_socket < 0),"ERROR on accept\n");



    ////////////////////////////////////////////////////////////////////////////////////
    //GET FRONTEND_PRIMARY SOCKET
	print_error(((frontend_primary_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1), "ERROR opening socket\n"); 
    //CONNECT TO FRONTEND_PRIMARY SOCKET
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(primary_server_port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
   
    print_error((connect(frontend_primary_socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
	//////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
  	//CREATE THREADS
    pthread_create(&thr_client_to_primary_server, NULL, client_to_primary_server,NULL);
    pthread_create(&thr_primary_server_to_client, NULL, primary_server_to_client,NULL);
    pthread_create(&thr_receive_new_primary, NULL, receive_new_primary,NULL);
    pthread_join(thr_client_to_primary_server,NULL);
    pthread_join(thr_primary_server_to_client, NULL);
    pthread_join(thr_receive_new_primary, NULL);

}



//Send the messages from the client to the server
void *receive_new_primary(void *arg){
	
	
	//Receive message
    while(1){	
    	//GET NEW PRIMARY
    	frontend_primary_socket = accept(sockfds, (struct sockaddr *)&cli_addr, &clilen);
        print_error((primary_server_port < 0),"ERROR on accept");

        pthread_cancel(thr_client_to_primary_server);
        pthread_cancel(thr_primary_server_to_client);

        pthread_create(&thr_client_to_primary_server, NULL, client_to_primary_server,NULL);
   		pthread_create(&thr_primary_server_to_client, NULL, primary_server_to_client,NULL);
   		//pthread_join(thr_client_to_primary_server,NULL);
    	//pthread_join(thr_primary_server_to_client, NULL);
   	};
}


//Send the messages from the client to the server
void *client_to_primary_server(void *arg){
	

	//Warn server of this port
	char payload[5];
    sprintf(payload, "%d", get_frontend_port());
	
	//Receive message
    packet message;
    while(1){	
    	
        if(receive(client_frontend_socket, &message)){
            if(send_packet(frontend_primary_socket,message.type, message.sqn, message.len, message.timestamp,message.payload)){
                

                if(message.type == INIT_USER){
                    sleep(0.5);
                    send_packet(frontend_primary_socket, UPDATE_PORT, 1, strlen(payload)+1 , getTime(), payload );
                }

                free(message.payload);
              
            }
        }
    	
        
    	
    	

    	
   	};
}


//Send the messages from the server to client
void *primary_server_to_client(void *arg){
	
	
	//Receive message
    packet message;
    while(1){
    	if(receive(frontend_primary_socket, &message)){
            if(send_packet(client_frontend_socket,message.type, message.sqn, message.len, message.timestamp,message.payload)){
                free(message.payload);
            }
        
        }
    	
    	
    	
   	};
}

//return the frontend port to the user
int get_frontend_port(){

	//frontend might not have found the port yet, waiting for it to find
	while(frontend_port == -1);

	


	return frontend_port;
    
}
 