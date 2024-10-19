# Multi-Threaded-Network-Server-for-Pattern-Analysis
## Objective
This project aims to create a high-performance multi-threaded network server capable of managing incoming connections, processing text data, and analysing patterns within the data.

## Setup
I am using the Gutenberg Project (https://www.gutenberg.org) to obtain large text files for this project. <br/>
Books are downloaded in plain text format (UTF-8) and saved for testing, e.g. 'Great Expectations', 'The Adventures of Sherlock Holmes' and 'The Wonderful Wizard of Oz'.

To send these text files to the program, I am utilising the netcat tool (nc). To install the package on Linux, run `sudo apt-get install netcat`.<br/>
Using netcat to transmit a text file to the server, the following command is used:<br/>
`nc localhost <port> -i <delay> -q 0 < <filename>.txt`

## Multi-Threaded Network Server
To compile the source code, run:<br/>
`gcc -O2 -Wall -pthread server.c -o <output file name>`<br/>

To start the server, run:<br/>
`./<output file name> -l <listening port> -p "<search pattern>"`<br/>

#### Socket Programming
The server is written in C. It listens for incoming connections on the port specified in the command line prompt.<br/>
See https://www.geeksforgeeks.org/socket-programming-cc/ for the socket implementation tutorial.

#### Server Logic
A new thread is created for each incoming client connection. This approach allows multiple clients to connect simultaneously.<br/>
In each thread, **non-blocking reads** are implemented from the sockets to efficiently receive and store data in a global shared list.<br/>

### Global Shared List
#### Purpose
The Shared List stores and links every line read across all threads, keeping track of the history of how data has arrived and been processed.<br/>
A `pthread_mutex` has been implemented to avoid race conditions across concurrent client threads when writing to the list.

#### Tasks
1. _Managing multiple readers_ (for each incoming read or line, a new node is created and added to the shared list.)
2. _Keeping track of each book_ (a book head pointer is embedded in each thread data and a `book_next` pointer is added to each list node on the shared list. This ensures book lines in the correct order.)
3. _Printing a book_ (output each received book in the same order as the client connection was accepted and ensure contents match the client book.)

### Multithreaded Frequency Analysis
#### Purpose
After adding each line, the server checks if it contains a specified search pattern. If a match is found, the program will track the number of lines that contain the search pattern and update the `next_frequent_search` pointer to navigate these lines.<br/>

#### Race Conditions
1. When accessing from the shared list, a `pthread_mutex` is utilized to ensure only one analysis thread is reading / one client thread is writing to the list at any given time. <br/>

2. The pattern frequency analysis is handled by multiple concurrent threads that output the analysis results at regular incremental intervals, i.e. every 2 seconds (first thread), 4 seconds (second thread), and so on.<br/>
If there are **competing threads** to print to the console, **only the first analysis thread that started executing will have printing rights**. 
This is established using:<br/>

- pthread_mutex
- pthread_cond
- `first_thread_printing` conditional variable

#### Output
The thread orders the book with the highest pattern occurrence frequency first and prints to the console in the following format:<br/>
`{rank} --> Book: {book_title}, Pattern: "{search_pattern}", Frequency: {frequency_count}`

## Testing
1. The server scales to over 10 concurrent client connections to ensure robustness.
2. Server outputs each received book by traversing the list from the book's header via "book_next" pointer. Each line is written to the respective filename "book_xx.txt", where xx is the order at which the client connection was accepted. The contents of the output book is compared to the original book.

#### Authored by Jingyi Qiu
