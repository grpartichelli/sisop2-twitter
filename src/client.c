
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

#include "utils.c"

void quit_signal();
void intHandler(int dummy);
void *client_input(void *arg);
int get_command(char* buffer);
void *client_display(void *arg);

int sockfd;
int sqncnt = 0;

// Gerenciador de comunicação: OK?
// Gerenciador de notificações: TO-DO
// Gerenciador de interface: ???

void quit_signal(){ //Não sei se devemos manter assim.
    send_packet(sockfd,CMD_QUIT,++sqncnt,0,0,"");
    receive_and_print(sockfd);
    close(sockfd);
    exit(1);
}

//detecting ctrl+c
void intHandler(int dummy) {
    quit_signal();
}

//Get client input and send to server
void *client_input(void *arg) 
{
    int n, sockfd  = *(int *) arg, flag, command;
    char in_buffer[BUFFER_SIZE];

    signal(SIGINT, intHandler); //detect ctrl+c

    while(1)
    {
        
        bzero(in_buffer, BUFFER_SIZE);
        printf("Enter a command: \n");
        
        if (!fgets(in_buffer, BUFFER_SIZE, stdin))
            command = CMD_QUIT;
        else
            command = get_command(in_buffer);

        switch(command){
            case CMD_QUIT:
                quit_signal();
                exit(1);
                break;
            case CMD_SEND:
                // If the message is at max 128 characters long, includes "SEND " and \n
                if (strlen(in_buffer) <= 134) 
                    send_packet(sockfd, CMD_SEND, ++sqncnt, strlen(in_buffer)-5, getTime(), in_buffer+5*sizeof(char));
                else
                    printf("Your message is too long, please use at maximum 128 caracters.\n");
                break;
            
            case CMD_FOLLOW:
           
                if(in_buffer[7]=='@'){
                    
                    if(strlen(in_buffer)-8 <=20 && strlen(in_buffer)-8 >=4)
                        send_packet(sockfd, CMD_FOLLOW, ++sqncnt, strlen(in_buffer)-7, getTime(), in_buffer+7*sizeof(char));             
                    else
                        printf("The username must be between 4 and 20 characters long.\n");
                }
                else
                    printf("An @ should be included before the username.\n");
           
                break;

            default:
                printf("Unknown command, try SEND <message> or FOLLOW <username>. To quit, use CTRL+C/D.\n");
            

        }

    }
        
}

int get_command(char* buffer)
{
    //////////////////////////////////////////////////////////
    // Checking for SEND command
    if(!strncmp(buffer,"SEND ", 5))
        return CMD_SEND;
   
    //////////////////////////////////////////////////////////
    // Checking for FOLLOW command 
    if(!strncmp(buffer,"FOLLOW ", 7))
        return CMD_FOLLOW;

    // If the command is neither SEND nor FOLLOW, returns -1
    return -1; 

}

/*
//Receive server response and display to user
void *client_display(void *arg) {
    int n, sockfd  = *(int *) arg;

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    
    while(1){
        
        n = read(sockfd, buffer, BUFFER_SIZE);
        while(n < 0){
            n = read(sockfd, buffer, BUFFER_SIZE);
        }



        printf("%s\n",buffer);
    }
}
*/

void *client_display(void *arg) {

   // TO-DO: Login code. profile.online++.

   int newsockfd = *(int *) arg;
   int flag = 1;
   packet message;


   while(flag){
      
      //READ
      receive(newsockfd, &message);
      switch(message.type)
      {
         case CMD_QUIT:
            printf("Numero maximo de acessos excedido.\n");
            close(newsockfd);
            exit(1);
         break;     
         case CMD_FOLLOW:
            printf("%s\n", message.payload);
         break;   
      }
      
      free(message.payload);
   }

}

void validate_user(char *profile){

    if(!(strlen(profile) <=20 && strlen(profile) >=4)){
        fprintf(stderr,"The username must be between 4 and 20 characters long.\n");
        exit(1);
    }

    if(profile[0] != '@'){
        fprintf(stderr,"The username must start with @ \n");
        exit(1);
    }

}

void load_user(char *profile){
    //Send profile that connected to server
    send_packet(sockfd, INIT_USER, ++sqncnt, strlen(profile)+1, getTime(), profile);
   
}


int main(int argc, char *argv[])
{
    pthread_t thr_client_input, thr_client_display;

    int  n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char profile[20];
    int port;

    //Check if correct input
    if (argc < 4) {
		fprintf(stderr,"Usage: ./app_cliente <profile> <server_address> <port>\n");
		exit(1);
    }

	

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


    //TODO Check for correct USER
    strcpy(profile,argv[1]);
    validate_user(profile);
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
    
    load_user(profile);

    pthread_create(&thr_client_input, NULL, client_input, &sockfd);
    pthread_create(&thr_client_display, NULL, client_display, &sockfd);
    
    pthread_join(thr_client_input, NULL);    
    pthread_join(thr_client_display, NULL);  

	close(sockfd);
    return 0;
}