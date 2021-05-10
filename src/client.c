
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

#include "communication.c"
#include "error_handler.c"
#include "frontend.c"

void quit_signal();
void intHandler(int dummy);
void *client_input(void *arg);
int get_command(char* buffer);
void *client_display(void *arg);

int sockfd;
int sqncnt = 0;



//detecting ctrl+c
void intHandler(int dummy) {
    quit_signal();
}


void quit_signal(){ //When we want to quit, send this to the server
    packet message;

    send_packet(sockfd,CMD_QUIT,++sqncnt,0,0,"");
    receive_and_print(sockfd);


    system("clear"); 
    printf("Cliente encerrou sua sessão.\n");

    close(sockfd);

    
    
    exit(1);
}


//Get client input and send to server
void *client_input(void *arg) 
{
    int n, sockfd  = *(int *) arg, flag, command;
    char in_buffer[BUFFER_SIZE];

    signal(SIGINT, intHandler); //detect ctrl+c

    while(1)
    {
        //GET THE COMMAND
        bzero(in_buffer, BUFFER_SIZE);
        if (!fgets(in_buffer, BUFFER_SIZE, stdin))
            command = CMD_QUIT;
        else
            command = get_command(in_buffer);

        switch(command){
            case CMD_QUIT:   
                quit_signal();
                break;

            case CMD_SEND:
                // If the message is at max 128 characters long, includes "SEND " and \n
                if (strlen(in_buffer) <= 134) 
                    send_packet(sockfd, CMD_SEND, ++sqncnt, strlen(in_buffer)-5, getTime(), in_buffer+5*sizeof(char));
                else
                    printf("Your message is too long, please use at maximum 128 caracters.\n");
                break;
            
            case CMD_FOLLOW:
                //SEND FOLLOW TO SERVER
                send_packet(sockfd, CMD_FOLLOW, ++sqncnt, strlen(in_buffer)-7, getTime(), in_buffer+7*sizeof(char));             
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

void *client_display(void *arg) {

   

   int newsockfd = *(int *) arg;
   int flag = 1;
   packet message;

   while(flag){
      
      
      receive(newsockfd, &message);
      switch(message.type)
      {
         case CMD_QUIT: //Server asks us to quit if we exceed the number of users
            printf("Numero maximo de acessos excedido.\n");
            close(newsockfd);
            exit(1);
         break;     
         case CMD_FOLLOW: //Simply print the follow message sent by the server. (If it was sucessfull or not)
            printf("%s\n", message.payload);
         break;   

         case CMD_SEND: //Simply print if the SEND was successfull
            printf("%s\n", message.payload);
         break; 

         case NOTIF: //Simply print the notification sent by the server
            printf("%s\n", message.payload);
         break;  
    
      }
      
      free(message.payload);
   }

}

void validate_user(char *profile){
    //Test if username is the correct size and has a @
    print_error(!(strlen(profile) <=20 && strlen(profile) >=4),"The username must be between 4 and 20 characters long.\n"); 
    print_error((profile[0] != '@'),"The username must start with @ \n"); 
}

void load_user(char *profile){
    //Send profile that connected to server
    send_packet(sockfd, INIT_USER, ++sqncnt, strlen(profile)+1, getTime(), profile);
   
}


int main(int argc, char *argv[])
{
    pthread_t thr_client_input, thr_client_display,thr_frontend_run;

    int  n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char profile[20];
    int primary_port;
    int frontend_port;

    //Check if correct input
    print_error((argc < 4),"Usage: ./app_cliente <profile> <server_address> <port>\n");
    
    //Check for correct HOST
    server = gethostbyname(argv[2]);
    print_error((server == NULL), "ERROR, no such host\n");
   
    //GET STARTING PRIMARY RM PORT
    primary_port = atoi(argv[3]);
    
    
    //GET USER
    strcpy(profile,argv[1]);
    validate_user(profile);
    

    //DISPLAY DETAILS
    printf("Profile: %s\n",profile);
    printf("Use the commands FOLLOW and SEND to comunicate with the server.\n");

    //OPEN SOCKET
    print_error(((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1), "ERROR opening socket\n"); 
    
    //RUN THE FRONTEND
    struct frontend_params params;
    strcpy(params.host,argv[2]);
    params.primary_port = primary_port;
    
    pthread_create(&thr_frontend_run, NULL, frontend_run, &params);
    frontend_port = get_frontend_port();

    //CONNECT TO FRONT END SOCKET
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(frontend_port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
  
    print_error((connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) , "ERROR connecting\n"); 
	
    
    //SEND USER INITIALIZATION TO SERVER
    load_user(profile);

    //CREATE THREADS FOR USER COMMAND INPUT AND DISPLAY NOTIFICATIONS
    pthread_create(&thr_client_input, NULL, client_input, &sockfd);
    pthread_create(&thr_client_display, NULL, client_display, &sockfd);
    
    pthread_join(thr_client_input, NULL);    
    pthread_join(thr_client_display, NULL);  
    
    
    pthread_join(thr_frontend_run, NULL);  
	close(sockfd);
    return 0;
}