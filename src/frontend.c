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

int port = -1;
void *frontend_run(void *arg){

	struct hostent *server =   (struct hostent *) arg;

	struct sockaddr_in serv_addr,cli_addr;;
    int sockfd,yes=1, newsockfd,clilen,found_port=0;
	int aux_port = 4999;//Default port
    
    //CREATE SOCKET
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
    //OPEN SOCKET  
    print_error((sockfd < 0),"ERROR opening socket\n"); 

    //FINDING AN AVAILABLE PORT
    while(!found_port){
    	aux_port++;
	    //Initializing structure
	    bzero((char *) &serv_addr, sizeof(serv_addr)); 
	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_addr.s_addr = INADDR_ANY;
	    serv_addr.sin_port = htons(aux_port);
	    

	    //BIND TO HOST
	    print_error((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1),"ERROR on setsockopt\n"); 
	  	
	  	
	  	//If the binding fails, we try again in another port.
	    if( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
	    	//If the binding works,set the port
	  		found_port = 1;
	    }

	}
	
	port = aux_port;


	
	//LISTEN
   	listen(sockfd,5);
   	clilen = sizeof(cli_addr);
   	//ACCEPT
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    print_error((newsockfd < 0),"ERROR on accept\n");


    //Receive message
    packet message;
    while(1){
   	 	
    	receive(newsockfd, &message);
    	
    	if(message.type == CMD_QUIT){
    		exit(1);
    	}

    	printf("%s\n", message.payload);
    	free(message.payload);
   	};
   
    

}


//return the frontend port to the user
int get_frontend_port(){

	//frontend might not have found the port yet, waiting for it to find
	while(port == -1);

	printf("%d\n",port);
	return port;
    
}
 