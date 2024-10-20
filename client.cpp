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
#include <vector>

/* Constants */
#define RCVBUFSIZE 512         // The receive buffer size
#define SNDBUFSIZE 512         // The send buffer size

struct MessageRequest {
    // std::vector<std::string> payLoad;
    std::string requestType;
};

void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}

void msg_display(int &option) {
    std::cout << "Message Options" << std::endl;
    std::cout << "1. List Files" << std::endl;
    std::cout << "2. List Difference" << std::endl;
    std::cout << "3. Pull Changes" << std::endl;
    std::cout << "4. Leave" << std::endl;
    std::cout << std::endl;
    std::cout << "Enter option: " << std::endl;

    std::cin >> option;
}

MessageRequest createMessage(const std::string &type) {
    MessageRequest req;
    req.requestType = type;
    return req;
}


/* The main function */
int main(int argc, char *argv[]) {
    int clientSock;                 // Socket descriptor
    struct sockaddr_in serv_addr;   // The server address

    std::string studentName;        // Your Name

    char rcvBuf[RCVBUFSIZE];        // Receive Buffer
    

    // studentName = argv[1];          // Assign student's name from command line
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
    int returnBytes = 0;      // Initialize number of received bytes
    std::string serverResponse; // String to store the server response

    while (true) {
        int option;
        MessageRequest req;

        msg_display(option);

        switch(option) {
            case 1:
                req = createMessage("LIST");
                std::cout << req.requestType << std::endl;
                send(clientSock, &req, sizeof(req), 0);
                break;
            case 2:
                req = createMessage("DIFF");
                std::cout << req.requestType << std::endl; 
                break;
            case 3:
                req = createMessage("PULL");
                std::cout << req.requestType << std::endl;
                break;
            case 4:
                req = createMessage("LEAVE");
                std::cout << req.requestType << std::endl;
                break;
            default:
                std::cout << "Invalid option. Selection a message between" << std::endl;
                continue;
        }
    }

    if (returnBytes < 0) {
        fatal_error("recv() returned an error");
    }

    // Print the received message from the server (list of files)
    std::cout << serverResponse << std::endl;

    // Close the client socket after the interaction
    close(clientSock);
    return 0;
}
