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

The server is written in C. It listens for incoming connections on the port specified in the command line prompt.<br/>
See https://www.geeksforgeeks.org/socket-programming-cc/ for socket implementation tutorial.
The program creates a new thread for each incoming connection to handle client communication. This approach allows multiple clients to connect simultaneously.<br/>
In each thread, non-blocking reads are implemented from the sockets to efficiently receive and store data in a global shared list.<br/>
Every line read is linked into that shared list that is the same across all threads.

### Shared List Management
This involves several tasks:
- _Managing multiple readers_ (for each incoming read or line, a new node is created and added to the shared list.)
- _Keeping track of each book_ (a book head pointer is embedded to each thread data and a `book_next` pointer is added to each list node on the shared list. This ensures book lines in the correct order.)
- _Printing a book_ (output each received book in the same order as the client connection was accepted and ensure contents match the client book.)

### Multithreaded Frequency Analysis

After each line is added, the server checks if it contains a specified search pattern. If a match is found, this triggers the program to track the number of lines that contain the search pattern and create a list to navigate these lines.<br/>
The pattern frequency analysis is handled by multiple concurrent threads that output the analysis results at regular intervals, i.e. every 5 seconds. The thread orders the book with the highest pattern occurence frequency first and prints to console in the following format:<br/>
`{rank} --> Book: {book_title}, Pattern: "{search_pattern}", Frequency: {frequency_count}`

## Testing
The server scales to over 10 concurrent client connections.

#### Authored by Jingyi Qiu
