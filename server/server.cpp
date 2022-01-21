#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#define MAX 80
#define SA struct sockaddr
#define PLAYERS_IN_ONE_GAME 2



// Function designed for chat between client and server.
void game(int *connfd)
{
    char buff[MAX];
    int n;
    // infinite loop for chat
    for (;;) {
        for (int i = 0; i < sizeof(connfd)/sizeof(connfd[0]); i++)
        {
            bzero(buff, MAX);
            printf("[INFO] Waiting for a message from client %i\n", i);   
            read(connfd[i], buff, sizeof(buff)); // read the message from client and copy it in buffer
            
            printf("From client %d: %s\n", i, buff); // print buffer which contains the client contents
            bzero(buff, MAX);
            n = 0;
            
            // while ((buff[n++] = getchar()) != '\n'); // copy server message in the buffer
    

            strncpy(buff, "[ACCEPT] Move accepted! Please wait for another player to move.\n", MAX); // fill up the buff array
            write(connfd[i], buff, sizeof(buff)); // and send that buffer to client
    
            
        }
        if (strncmp("exit", buff, 4) == 0) { // if msg contains "Exit" then server exit and chat ended.
                printf("Server Exit...\n");
                break;
        }
    }
}
   
void *createInstance(void *threadid)
{
    int sockfd, len;
    struct sockaddr_in servaddr, cli;
    
    char buff[MAX];
    int connfd[PLAYERS_IN_ONE_GAME];
    long tid = (long)threadid;
    printf("THREADID: %ld\n", tid);

    // Accept the data packet from client and verification
    for (int i = 0; i < sizeof(connfd)/sizeof(connfd[0]); i++)
    {
        printf("[THREAD %ld] INFO] Waiting for a new player...\n", tid);
        connfd[i] = accept(sockfd, (SA*)&cli, (unsigned int*)&len);
        if (connfd[i] < 0) {
            printf("[THREAD %ld] [ERROR] server accept failed...\n", tid);
            exit(0);
        }
        else
            printf("[THREAD %ld] [LOG] Server accept the client %d:%d on server %d\n", tid, i, connfd[i], sockfd);

            strncpy(buff, "[ACCEPT] Successfuly connected to the new game as a player!\n", MAX); // fill up the buff array
            write(connfd[i], buff, sizeof(buff)); // and send that buffer to client
    }
   
    // game function
    game(connfd);

}

// Driver function
int main()
{
    int PORT = 8080;
    int GAMES = 2;
    pthread_t threads[GAMES];
    int rc;


    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("[ERROR] Socket %d creation failed...\n", tid, sockfd);
        exit(0);
    }
    else
        printf("[LOG] Socket %d successfully created..\n", tid, sockfd);
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
   
    // Binding newly created socket to given IP and verification
    while ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("[WARNING] Socket bind to port %d failed, trying new PORT...\n", tid, PORT);
        PORT++;
        servaddr.sin_port = htons(PORT);
        // exit(0);
    }
    printf("[LOG] Socket successfully binded to port %d..\n", tid, PORT);
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("[ERROR] Listen failed...\n", tid);
        exit(0);
    }
    else
        printf("[LOG] Server listening..\n", tid);
    len = sizeof(cli);
   
    //creating new instances
    for( int i = 0; i < GAMES; i++ ) {
      printf("[LOG] Creating a new game instance %d\n", i);
      rc = pthread_create(&threads[i], NULL, createInstance, (void *)i);
      
      if (rc) {
         printf("[ERROR] Unable to create a new game instance %d\n", rc);
         exit(-1);
      }
   }

    // After chatting close the socket
    close(sockfd);
   pthread_exit(NULL);

    
}