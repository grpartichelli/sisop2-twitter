#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 4000

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int  n;
   
   //Create socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   //Initializing structure
   bzero((char *) &serv_addr, sizeof(serv_addr));

   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(PORT);
   
   // BIND TO HOST
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }

	//LISTEN
   listen(sockfd,5);
   clilen = sizeof(cli_addr);

   //ACCEPT
   newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	
   if (newsockfd < 0) {
      perror("ERROR on accept");
      exit(1);
   }
	   
    

   printf("Server started correctly.\n");
   while(1){
	   	
	  
	   //READ
	   bzero(buffer,256);
	   n = read( newsockfd,buffer,255 );
	   if (n < 0) {
	      perror("ERROR reading from socket");
	      exit(1);
	   }
	   printf("Here is the message: %s",buffer);
	   
	   //WRITE
	   n = write(newsockfd,"I got your message",18); 
	   if (n < 0) {
	      perror("ERROR writing to socket");
	      exit(1);
	   }
    }

    printf("Server ended successfully\n");

   return 0;
}