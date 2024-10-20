/*///////////////////////////////////////////////////////////
*
* FILE:        client.cpp
* AUTHOR:      
* PROJECT:     CNT 4007 Project 2 - Professor Traynor
* DESCRIPTION: Network Client Code (C++)
*
*////////////////////////////////////////////////////////////

#include <iostream>            // For C++ I/O
#include <sys/socket.h>        // For socket(), connect(), send(), and recv()
#include <arpa/inet.h>         // For sockaddr_in and inet_addr()
#include <cstdlib>             // For general-purpose functions like exit()
#include <unistd.h>            // For close()
#include <cstring>             // For C-style string manipulation (memset)

/* Constants */
#define RCVBUFSIZE 512         // The receive buffer size
#define SNDBUFSIZE 512         // The send buffer size

void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}

/* The main function */
int main(int argc, char *argv[]) {
    int clientSock;                 // Socket descriptor
    struct sockaddr_in serv_addr;   // The server address

    std::string studentName;        // Your Name

    char rcvBuf[RCVBUFSIZE];        // Receive Buffer
    
    // Check if proper command-line arguments are provided
    if (argc != 2) {
        std::cerr << "Incorrect input format. The correct format is:\n\tnameChanger your_name\n";
        exit(1);
    }

    studentName = argv[1];          // Assign student's name from command line
    memset(rcvBuf, 0, RCVBUFSIZE);  // Clear receive buffer

    // Create a new TCP socket
    if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fatal_error("Socket creation failed");
    }

    // Construct the server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));   // Clear server address structure
    serv_addr.sin_family = AF_INET;             // Internet address family
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP (localhost)
    serv_addr.sin_port = htons(9090);           // Server port number

    // Establish connection to the server
    if (connect(clientSock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fatal_error("Connection to server failed");
    }

    // Send the student's name to the server
    int msglen = studentName.length();
    if (send(clientSock, studentName.c_str(), msglen, 0) != msglen) {
        fatal_error("send() sent an unexpected number of bytes");
    }

    // Receive the message back from the server
    int returnBytes = recv(clientSock, rcvBuf, RCVBUFSIZE - 1, 0);
    if (returnBytes < 0) {
        fatal_error("recv() returned less than 0 bytes");
    }

    // Print the received message from the server
    std::cout << "Message received from server: " << std::string(rcvBuf, returnBytes) << std::endl;

    // Close the client socket after the interaction
    close(clientSock);
    return 0;
}
