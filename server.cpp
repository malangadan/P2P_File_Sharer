/*/////////////////////////////////////////////////////////// 
 * FILE:        server.cpp
 * AUTHOR:      Your Name Here
 * PROJECT:     CNT 4007 Project 1 - Professor Traynor
 * DESCRIPTION: Network Server Code (C++)
 *///////////////////////////////////////////////////////////*/

#include <iostream>        // For input/output in C++
#include <sys/socket.h>    // For socket(), connect(), send(), and recv()
#include <arpa/inet.h>     // For sockaddr_in and inet_addr()
#include <cstdlib>         // For general-purpose functions like exit()
#include <unistd.h>        // For close()
#include <cstring>         // For string manipulation functions
#include <openssl/evp.h>   // For OpenSSL EVP digest libraries/SHA256
#include <string>          // For std::string
#include <algorithm>

#define RCVBUFSIZE 512     // The receive buffer size
#define SNDBUFSIZE 512     // The send buffer size
#define BUFSIZE 40         // Your name can be as many as 40 chars
#define MAXPENDING 10

void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}


void executeCommand(std::string &nameBuf) {
    std::cout << "before execution" << std::endl;
    std::cout << nameBuf << std::endl;
    nameBuf.erase(std::remove_if(nameBuf.begin(), nameBuf.end(), ::isspace), nameBuf.end());
    std::cout << nameBuf << std::endl;
    if (nameBuf == "LIST") { 
        nameBuf = "I will list how the music files now:";
    } else {
        nameBuf = "Unknown command";
    }
}

int main(int argc, char *argv[]) {
    int serverSock;                // Server Socket
    int clientSock;                // Client Socket
    struct sockaddr_in changeServAddr; // Local address
    struct sockaddr_in changeClntAddr; // Client address
    unsigned short changeServPort = 9090; // Server port (defined as 9090)
    unsigned int clntLen;          // Length of address data struct

    std::string nameBuf(BUFSIZE, 0);   // Buffer to store name from client
    unsigned char md_value[EVP_MAX_MD_SIZE];  // Buffer to store change result
    EVP_MD_CTX *mdctx;              // Digest data structure declaration
    const EVP_MD *md;               // Digest data structure declaration
    int md_len;                     // Digest data structure size tracking

    // Create new TCP Socket for incoming requests
    if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fatal_error("Socket Failed");
    }

    // Construct local address structure
    memset(&changeServAddr, 0, sizeof(changeServAddr));
    changeServAddr.sin_family = AF_INET;
    changeServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    changeServAddr.sin_port = htons(changeServPort);

    // Bind to local address structure
    if (bind(serverSock, (struct sockaddr *) &changeServAddr, sizeof(changeServAddr)) < 0) {
        fatal_error("bind() failed");
    }

    // Listen for incoming connections
    if (listen(serverSock, MAXPENDING) < 0) {
        fatal_error("listen() failed");
    }

    // Loop server forever
    while (true) {
        clntLen = sizeof(changeClntAddr);
        // Accept incoming connection
        if ((clientSock = accept(serverSock, (struct sockaddr *) &changeClntAddr, &clntLen)) < 0) {
            fatal_error("accept() failed");
        }
        // Extract Your Name from the packet, store in nameBuf
        int recvLen = recv(clientSock, &nameBuf[0], BUFSIZE - 1, 0);
        if (recvLen < 0) {
            fatal_error("recv() failed");
        }

        nameBuf[recvLen] = '\0';  // Null-terminate the received string
        executeCommand(nameBuf); // Execute user specified action
        std::cout << "execution done" << std::endl;

        // Send the same nameBuf string back to the client
        if (send(clientSock, nameBuf.c_str(), nameBuf.size(), 0) != nameBuf.size()) {
            fatal_error("Send failed");
        }

        close(clientSock);  // Close client connection after handling
    }

    return 0;
}
