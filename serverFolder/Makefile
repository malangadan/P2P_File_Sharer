#################################################################
##
## FILE:    Makefile
## PROJECT: CNT 4007 Project 1 - Professor Traynor
## DESCRIPTION: Compile server only
##
#################################################################

CC=g++

OS := $(shell uname -s)

# Extra LDFLAGS if Solaris
ifeq ($(OS), SunOS)
    LDFLAGS=-lsocket -lnsl
endif

all: server 

server: server.cpp
	$(CC) server.cpp -o server -lcrypto

clean:
	rm -f server *.o
