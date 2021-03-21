#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 4000
#define MAX_CLIENTS 500


void *handle_client(void *arg) {
   char buffer[256];
   int n, newsockfd = *(int *) arg;

   int flag = 1;
   while(flag){
           
      //READ
      bzero(buffer,256);

      n = read( newsockfd,buffer,255 );
      while(n < 0){
         n = read( newsockfd,buffer,255 );
      }

      printf("Here is the message: %s",buffer);


      
      if(!strcmp(buffer,"quit")){ //User warning to quit
         printf("\nUser quit\n");
         write(newsockfd,"Successfully quit.",18); 
         flag = 0;

      } else{

         //WRITE
         n = write(newsockfd,"I got your message",18); 
         if (n < 0) {
            printf("ERROR writing to socket");
            exit(1);
         }
      }

      

      
      ////////////////////////////////////////////
   }

   

}


int main( int argc, char *argv[] ) {

   pthread_t client_pthread[MAX_CLIENTS];
   int sockfd, newsockfd, portno, clilen;
   struct sockaddr_in serv_addr, cli_addr;
   
   //Create socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      printf("ERROR opening socket\n");
      exit(1);
   }
   
   //Initializing structure
   bzero((char *) &serv_addr, sizeof(serv_addr));

   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(PORT);
   


   // BIND TO HOST
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      printf("ERROR on binding.\n");
      exit(1);
   }

   printf("Server started correctly.\n");

	//LISTEN
   listen(sockfd,5);
   clilen = sizeof(cli_addr);

   int i=0;
   while(1){
      
      //ACCEPT
      newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
      if (newsockfd < 0) {
         printf("ERROR on accept");
         exit(1);
      }


      if(pthread_create(&client_pthread[i], NULL, handle_client, &newsockfd) != 0 ){
         printf("Failed to create thread");
         exit(1);
      }

      
      i++;
   }
   

	  
   close(sockfd);
   printf("Server ended successfully\n");

   return 0;
}