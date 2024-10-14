#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For read(), write(), close()
#include <netinet/in.h>   // For struct sockaddr_in
#include <sys/socket.h>   // For socket(), bind(), listen(), etc.
#include <arpa/inet.h>    // For htonl(), htons(), inet_addr(), etc.
#define BACKLOG 128
#define MAXLINE 512 // Buffer size for incoming text lines
#define SA struct sockaddr

struct Node {
    char line[MAXLINE];
    struct Node* next;
    struct Node* book_next;
};

int log_file(char buffer[]){
    FILE *logFile = fopen("server_log.txt", "a");
    if(logFile == NULL){
        fprintf(stderr, "Error opening server log file.\n");
        return -1;
    }

    time_t now;
    time(&now);
    char *timestamp = ctime(&now);

    // Remove the newline from the timestamp (ctime adds one at the end)
    timestamp[strlen(timestamp) - 1] = '\0';

    fprintf(stdout, "[%s] %s\n", timestamp, buffer);
    fprintf(logFile, "[%s] %s\n", timestamp, buffer);

    fclose(logFile);
    return 0;
}

int read_lines(int connfd){
    char buffer[MAXLINE];
    int bytes_received;

    while((bytes_received = recv(connfd, buffer, MAXLINE, 0)) > 0){
        buffer[bytes_received] = '\0';
        log_file(buffer);
    }

    return 0;
}

int parse_arguments(int argc, char *argv[], char **port, char **pattern) {
    // Check if the right number of arguments is provided
    if (argc != 5) {
        printf("Usage: %s -l <listening port> -p <search pattern>\n", argv[0]);
        return -1; // Error code
    }

    // Iterate through the arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                *port = argv[i + 1]; // Get the port number following -l
            } else {
                printf("Error: -l flag requires an argument.\n");
                return -1; // Error code
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                *pattern = argv[i + 1]; // Get the search pattern following -p
            } else {
                printf("Error: -p flag requires an argument.\n");
                return -1; // Error code
            }
        }
    }

    // Check if both arguments were provided
    if (*port == NULL || *pattern == NULL) {
        printf("Error: Both -l and -p flags must be provided.\n");
        return -1; // Error code
    }

    return 0; // Success
}


int main(int argc, char *argv[]) {

    char *port_str = NULL;
    char *pattern = NULL;

    // Function to parse socket port and string pattern arguments
    int result = parse_arguments(argc, argv, &port_str, &pattern);
    if (result != 0) {
        exit(0);
    }

    int port = atoi(port_str);

    // Create socket via TCP connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        printf("Socket creation failed...\n");
        exit(0);
    }
    else{
        printf("Socket creation successful...\n");
    }

    // Bind socket to the localhost on port 8080
    struct sockaddr_in serv_addr;
    socklen_t serv_len = sizeof(serv_addr); 

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(port); 

    if(bind(sockfd, (SA*)&serv_addr, serv_len) != 0){
        printf("Socket bind failed...\n");
        exit(0);
    }
    else{
        printf("Socket bind successful...\n");
    }

    // Server ready to listen for incoming connection from client
    if(listen(sockfd, BACKLOG) != 0){
        printf("Listen failed...client request overflow\n");
        exit(0);
    }
    else{
        printf("Server listening...\n");
    }

    // Accept packets from client
    struct sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr); 

    int connfd = accept(sockfd, (SA*)&client_addr, &cli_len);

    if(connfd < 0){
        printf("Client accept failed...\n");
        exit(0);
    }
    else{
        printf("Client accepted...\n");
    }

    // Handles logic...
    // Prints all line of incoming text file
    if(read_lines(connfd) < 0){
        printf("File read error...\n");
        exit(0);
    }
    else{
        printf("File read success...\n");
    }

    // Close socket connection
    close(sockfd);
}