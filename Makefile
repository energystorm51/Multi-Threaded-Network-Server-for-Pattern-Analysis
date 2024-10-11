CC=gcc
CFLAGS=-O2 -Wall

assignment3: server.c
    $(CC) $(CFLAGS) -o assignment3 server.c