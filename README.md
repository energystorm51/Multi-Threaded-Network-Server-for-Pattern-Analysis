# Multi-Threaded-Network-Server-for-Pattern-Analysis
## Objective
This project aims to create a high-performance multi-threaded network server capable of managing incoming connections, processing text data, and analysing patterns within the data.

## Setup
Use resources like the Gutenberg Project (https://www.gutenberg.org) to obtain large text files for this project. <br/>
Download plain text format books (UTF-8) and save them locally for later use.

To send these text files to the program, consider utilising the netcat tool (nc). <br/>
For instance, to transmit a text file to the server, you may use the following command:<br/>
`nc localhost 1234 -i <delay> < file.txt`

## Multi-Threaded Network Server
The server is written in C. It listens for incoming connections on port 12345.<br/>
See https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html for socket implementation document.

The program creates a new thread for each incoming connection to handle client communication. This approach allows multiple clients to connect simultaneously.<br/>
In each thread, non-blocking reads are implemented from the sockets to efficiently receive and store data in one shared data structure -  a list.<br/>
Every line read is linked into that shared list that is the same across all threads.

### Shared List Management
This involves several tasks:
- _Managing multiple readers_ (for each incoming read or line, a new node is created and added to the shared list.)
- _Keeping track of each book_ (embed a `book_next` pointer into the shared list along with a head pointer for each book. This ensures book lines in the correct order.)
- _Printing a book_ (output each received book correctly.)

### Multithreaded Analysis


## Testing
The server starts with: `./ server -l 12345 -p "happy"`, where 12345 is a listen port and "happy" is the search pattern.<br/>
The program will count how many times the string "happy" appears in the book and create a list to navigate the lines that contain it.

#### Authored by Jingyi Qiu
