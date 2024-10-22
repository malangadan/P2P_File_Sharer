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

all: client 

client: client.cpp
	$(CC) client.cpp -o client -lcrypto

clean:
	rm -f client *.o