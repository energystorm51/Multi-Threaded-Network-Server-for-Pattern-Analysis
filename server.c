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

// Structure to hold node data
struct Node {
    char text[MAXLINE];
    struct Node* next;
    struct Node* book_next;
    struct Node* next_frequent_search;
    int connection_order;
    char book_title[MAXLINE];
};

// Structure to hold connection data
struct Thread_Data {
    int connfd;
    int connection_order;
    struct Node* book_head;
    struct Node* book_tail;
};

// Map to hold book data and pattern frequency
struct Analysis_Data {
    int frequency;
    char book_title[MAXLINE];
};

// Global connection counter and mutex
int connection_counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t print_cond = PTHREAD_COND_INITIALIZER;
int first_thread_printing = 0;  // Flag to indicate if a thread is currently printing

// Global port and pattern declaration
char *port_str = NULL;
char *pattern = NULL;

// Global List Initialization
struct Node* head = NULL;
struct Node* tail = NULL;

// Pattern Search List Variables
struct Node* first_pattern_node = NULL;
struct Node* previous_pattern_node = NULL;

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


// Function to manage Shared List
void update_shared_list(char buffer[], struct Thread_Data *data, int pattern_found){

    // Create node for new line read
    struct Node* new_read = malloc(sizeof(struct Node));
    strcpy(new_read->text, buffer);
    new_read->next = NULL;
    new_read->book_next = NULL;
    new_read->next_frequent_search = NULL;
    new_read->connection_order = data->connection_order;

    // Case 1: First read - empty list, set both head and tail to the new node
    if(head == NULL && tail == NULL){
        head = new_read;
        tail = new_read;
    }

    // Case 2: Single node, point head to new node and update tail
    else if(head == tail){
        head->next = new_read;

        if(head->connection_order == new_read->connection_order){
            head->book_next = new_read;
        }
        else{
            head->book_next = NULL;
        }
        
        tail = new_read;
    }

    // Case 3: Multiple nodes present, update tail
    else{
        tail->next = new_read;

        if(tail->connection_order == new_read->connection_order){
            tail->book_next = new_read;
        }
        else{
            tail->book_next = NULL;
        }

        tail = new_read;
    }

    // Update the book list of individual threads
    // Similar logic to updating the global shared list
    if(data->book_head == NULL){
        strcpy(new_read->book_title, new_read->text); // copy first line of book read to the 'title'
        data->book_head = new_read;
        data->book_tail = new_read;
    }

    else if(data->book_head == data->book_tail){
        strcpy(new_read->book_title, data->book_head->text); // copy line of book head to current book title
        data->book_head->book_next = new_read;
        data->book_tail = new_read;
    }

    else{
        strcpy(new_read->book_title, data->book_head->text);
        data->book_tail->book_next = new_read;
        data->book_tail = new_read;
    }

    // Update the identified pattern pointer
    if(pattern_found){

        // Track the first identified node in the first pattern occurence
        if(previous_pattern_node == NULL){
            previous_pattern_node = new_read;
            first_pattern_node = new_read;
        }
        // Update the last identified node to point to new node
        // Set new node as the last identified node
        else{
            previous_pattern_node->next_frequent_search = new_read;
            previous_pattern_node = new_read;
        }
    }

}

void print_list(struct Node* list_head){
    struct Node* curr = list_head;
    fprintf(stdout, "> Result for [%s]\n", pattern);
    while(curr != NULL){
        fprintf(stdout, "%s\n", curr->text);
        curr = curr->next_frequent_search;
    }
}



// Logs read lines to both console and file
void log_file(char buffer[], int connection_order){
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
    fprintf(stdout, "[%s] Book0%d: %s\n", timestamp, connection_order, buffer);
    fprintf(logFile, "[%s] Book0%d: %s\n", timestamp, connection_order, buffer);

    fclose(logFile);
}



/*
    Write received book
*/
void write_book(struct Node* book_head, int connection_order){
    char filename[20];

    snprintf(filename, sizeof(filename), "book_0%d.txt", connection_order);

    // Format filename using the global connection counter
    FILE *book = fopen(filename, "w");
    if(book == NULL){
        fprintf(stderr, "Error writing to book...\n");
        return;
    }

    struct Node* curr = book_head;
    while(curr != NULL){
        // fprintf(stdout, "%s", curr->text);
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
    int pattern_found;

    printf("Handling client connection #0%d...\n", data->connection_order);

    // Continuously read every incoming book line
    while(1){
        bytes_received = recv(data->connfd, buffer, MAXLINE, 0);

        if(bytes_received > 0){
            // Received data from client
            buffer[bytes_received] = '\0';

            // Find pattern occurence in the buffer line
            if(strstr(buffer, pattern) == NULL){
                pattern_found = 0;
            }
            else{
                pattern_found = 1;
            }

            // Write line read to global logfile using mutex
            pthread_mutex_lock(&log_mutex);
            log_file(buffer, data->connection_order);
            pthread_mutex_unlock(&log_mutex);

            // Add new node to the shared list
            pthread_mutex_lock(&list_mutex);
            update_shared_list(buffer, data, pattern_found);
            pthread_mutex_unlock(&list_mutex);
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
    write_book(data->book_head, data->connection_order);
    print_list(first_pattern_node);
    free(data);  // Free allocated memory for thread data
    pthread_exit(NULL);
}

/*
    Analysis Thread Handling
*/
void *analyze_patterns(void *arg) {
    int sleep_time = *(int*)arg;

    while(1) {  // Infinite loop to keep the analysis thread running

        pthread_mutex_lock(&print_mutex);  // Lock the printing mutex

        // Wait if another thread is already printing
        while (first_thread_printing) {
            pthread_cond_wait(&print_cond, &print_mutex); 
        }

        first_thread_printing = 1; // Set the flag to indicate that this thread is now printing
        
        // Sleep for an interval
        sleep(sleep_time);

        pthread_mutex_lock(&list_mutex);  // Lock the shared list before accessing it
        int occurrence_count[128] = {0};
        char book_titles[128][MAXLINE];

        struct Node* current = first_pattern_node;

        // Traverse the shared list and count pattern occurrences
        while(current != NULL){
            occurrence_count[current->connection_order]++;  // Increment counter for this connection
            strcpy(book_titles[current->connection_order], current->book_title); // Store book title of this connection
            current = current->next_frequent_search; // Point to next line of matching pattern
        }
        pthread_mutex_unlock(&list_mutex);  // Unlock the shared list

        struct Analysis_Data ordered_frequency_list[128];

        // Order pattern occurrence using selection sort
        for(int i = 1; i <= connection_counter; i++){

            // Initialize position with largest frequency
            int max_pos = i;

            for(int j = 1; j <= connection_counter; j++){
                if(occurrence_count[j] > occurrence_count[max_pos]){
                    max_pos = j;
                }
            }

            // Store data in descending order, i.e. largest first
            ordered_frequency_list[i].frequency = occurrence_count[max_pos];
            strcpy(ordered_frequency_list[i].book_title, book_titles[max_pos]);

            // Reset max pos values to avoid duplicates
            occurrence_count[max_pos] = 0;
            strcpy(book_titles[max_pos], "");

        }

        // Output occurence results
        for(int i = 1; i <= connection_counter; i++){
            printf("{%d} --> Book: {%s}, Pattern: '{%s}', Frequency: {%d}\n", i, ordered_frequency_list[i].book_title, pattern, ordered_frequency_list[i].frequency);
        }

        // Reset the flag after printing is done
        first_thread_printing = 0;

        // Signal to other threads that printing is done
        pthread_cond_broadcast(&print_cond);
        pthread_mutex_unlock(&print_mutex);  // Unlock the printing mutex
        
    }

    pthread_exit(NULL);  
}


// Main Program
int main(int argc, char *argv[]) {

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
    * Creates multiple analysis threads
    */
    int thread_count = 2;
    pthread_t analysis_tid[thread_count]; // Four threads
    int thread_sleep[thread_count];

    for(int i = 0; i < thread_count; i++){
        thread_sleep[i] = (i+1)*2;
        if(pthread_create(&analysis_tid[i], NULL, analyze_patterns, &thread_sleep[i]) != 0){
            perror("Failed to create analysis thread...\n");
            exit(1);
        }
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
        connection_counter++;
        pthread_mutex_unlock(&counter_mutex);

        // Create thread to handle client
        pthread_t tid;
        struct Thread_Data *data = malloc(sizeof(struct Thread_Data));
        data->connfd = connfd;
        data->connection_order = connection_counter;
        data->book_head = NULL;
        data->book_tail = NULL;

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