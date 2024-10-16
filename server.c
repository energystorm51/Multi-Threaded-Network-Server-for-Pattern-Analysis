#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>        // For non-blocking mode
#include <unistd.h>       // For read(), write(), close()
#include <netinet/in.h>   // For struct sockaddr_in
#include <sys/socket.h>   // For socket(), bind(), listen(), etc.
#include <arpa/inet.h>    // For htonl(), htons(), inet_addr(), etc.

#define BACKLOG 128
#define MAXLINE 512       // Buffer size for incoming text lines
#define SA struct sockaddr

// Global connection counter and mutex
int connection_counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// Parses input arguments from the console
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

// List Node structure Definition
struct Node {
    char text[MAXLINE];
    struct Node* next;
    struct Node* book_next;
};

// Global List Initialization
struct Node* head = NULL;
struct Node* tail = NULL;

// Helper functions to manage Shared List
void update_shared_list(char buffer[]){

    // Case 1: First read - empty list, set both head and tail to the new node
    if(head == NULL && tail == NULL){
        struct Node* new_read = malloc(sizeof(struct Node));
        strcpy(new_read->text, buffer);
        new_read->next = NULL;
        new_read->book_next = NULL;

        head = new_read;
        tail = new_read;
    }

    // Case 2: Single node, point head to new node and update tail
    else if(head == tail){
        struct Node* new_read = malloc(sizeof(struct Node));
        strcpy(new_read->text, buffer);
        new_read->next = NULL;
        new_read->book_next = NULL;

        head->next = new_read;
        head->book_next = new_read;

        tail = new_read;
    }

    // Case 3: Multiple nodes present, update tail
    else{
        struct Node* new_read = malloc(sizeof(struct Node));
        strcpy(new_read->text, buffer);
        new_read->next = NULL;
        new_read->book_next = NULL;

        tail->next = new_read;
        tail->book_next = new_read;

        tail = new_read;
    }

}

void print_shared_list(){
    struct Node* curr = head;
    while(curr != NULL){
        fprintf(stdout, "%s\n", curr->text);
        curr = curr->next;
    }
}



// Logs read lines to both console and file
void log_file(char buffer[]){
    FILE *logFile = fopen("logfile.txt", "a");
    if(logFile == NULL){
        fprintf(stderr, "Error opening server log file.\n");
        return;
    }

    // Get current system timestamp
    time_t now;
    time(&now);
    char *timestamp = ctime(&now);

    // Remove the newline from the timestamp (ctime adds one at the end)
    timestamp[strlen(timestamp) - 1] = '\0';

    // Print to console and/or logfile
    // fprintf(stdout, "[%s] %s\n", timestamp, buffer);
    fprintf(logFile, "[%s] %s\n", timestamp, buffer);

    fclose(logFile);
}



// Structure to hold connection data
struct Thread_Data {
    int connfd;
    int connection_order;
};

/*
    Write received book
*/
void write_book(struct Node* book_head){
    FILE *book = fopen("test_01.txt", "w");
    if(book == NULL){
        fprintf(stderr, "Error writing to book...\n");
        return;
    }

    struct Node* curr = book_head;
    while(curr != NULL){
        fprintf(stdout, "%s", curr->text);
        fprintf(book, "%s", curr->text);
        curr = curr->book_next;
    }

    fclose(book);
}

/*
    Reads book line from the netcat client in a thread
*/ 
void *read_book_lines(void *arg){
    struct Thread_Data *data = (struct Thread_Data *)arg;
    char buffer[MAXLINE];
    int bytes_received;

    printf("Handling client connection #0%d...\n", data->connection_order);

    while(1){
        bytes_received = recv(data->connfd, buffer, MAXLINE, 0);

        if(bytes_received > 0){
            // Received data from client
            // Logs line read and add new node to the shared list
            buffer[bytes_received] = '\0';
            log_file(buffer);
            update_shared_list(buffer);
        }
        else if(bytes_received == 0){
            // Client closed connection
            printf("Client #0%d disconnected...\n", data->connection_order);
            break;
        }
        else if(errno == EAGAIN || errno == EWOULDBLOCK){
            // No data available, non-blocking mode is active
            // Add a sleep to prevent busy-waiting
            usleep(1000);  // Sleep for 1 millisecond
            continue;
        }
        else{
            // Some other error occurred
            perror("recv error");
            break;
        }
    }

    // Cleanup and exit thread
    close(data->connfd);
    write_book(head);
    free(data);  // Free allocated memory for thread data
    pthread_exit(NULL);
}



// Main Program
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

    // Bind socket to the localhost on specified port
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
        printf("Server listening on PORT %d...\n", port);
    }

    /*
    * Accept packets from clients in a loop
    * Creates thread for each client
    */

   while(1){
        struct sockaddr_in client_addr;
        socklen_t cli_len = sizeof(client_addr); 

        int connfd = accept(sockfd, (SA*)&client_addr, &cli_len);

        if(connfd < 0){
            printf("Client accept failed...\n");
            continue;
        }
        else{
            printf("Client accepted...\n");
        }

        // After accepting the connection, set connfd to non-blocking mode
        int flags = fcntl(connfd, F_GETFL, 0);
        fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

        // Increment global counter using mutex to track connection order
        pthread_mutex_lock(&counter_mutex);
        int current_order = ++ connection_counter;
        pthread_mutex_unlock(&counter_mutex);

        // Create thread to handle client
        pthread_t tid;
        struct Thread_Data *data = malloc(sizeof(struct Thread_Data));
        data->connfd = connfd;
        data->connection_order = current_order;

        int thread = pthread_create(&tid, NULL, read_book_lines, (void*)data);
        printf("Thread created...\n");

        if(thread != 0){
            printf("Thread create failed...\n");
            close(connfd);
            free(data);
            continue;
        }

        pthread_detach(tid);
        printf("Thread detached...\n");
   }

    return 0;
}