#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For read(), write(), close()
#include <netinet/in.h>   // For struct sockaddr_in
#include <sys/socket.h>   // For socket(), bind(), listen(), etc.
#include <arpa/inet.h>    // For htonl(), htons(), inet_addr(), etc.
#define PORT 8080
#define BACKLOG 128
#define SA struct sockaddr

int main() {
    // Create socket via TCP connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        printf("Socket creation failed...\n");
        exit(0);
    }
    else{
        printf("Socket creation successful...\n");
    }

    struct sockaddr_in serv_addr;
    socklen_t serv_len = sizeof(serv_addr); 

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    serv_addr.sin_port = htons(PORT); 

    // Connect to server
    if(connect(sockfd, (SA*)&serv_addr, serv_len) != 0){
        printf("Connection with server failed...\n");
        exit(0);
    }
    else{
        printf("Connected to server...\n");
    }

    // Handles logic...

    // Close socket connection
    close(sockfd);
}