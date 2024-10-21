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

enum RequestType {
    LIST = 1,
    DIFF = 2,
    PULL = 3,
    LEAVE = 4
};

struct MessageRequest {
    // std::vector<std::string> payLoad;
    uint8_t type;
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

uint8_t decodeType(RequestType type) {
    switch(type) {
        case LIST:
            return 1;
        case DIFF:
            return 2;
        case PULL:
            return 3;
        case LEAVE:
            return 4;
        default:
            std::cout << "Incorrect type" << std::endl;
            exit(1);
            break;
    }
}

MessageRequest createMessage(RequestType type) {
    // Decode type
    uint8_t reqType = decodeType(type);
    
    MessageRequest req;
    req.type = reqType;
    return req;
}

std::vector<std::string> getList(std::vector<uint8_t> &sendBuff, std::vector<uint8_t> &recvBuff, int &clientSock) {
    
    // Send message
    send(clientSock, sendBuff.data(), sendBuff.size(), 0);
    // Check for response from server

    // std::cout << static_cast<unsigned>(recvBuff[0]) << std::endl;

    int fileListLength = 0;
    if (recv(clientSock, recvBuff.data(), sizeof(fileListLength), 0) < 0) {
            fatal_error("Error getList (fnameLength): ");
    }

    fileListLength = static_cast<unsigned>(recvBuff[0]);
    std::cout << "File List Length: " << fileListLength << std::endl;
    std::vector<std::string> fileList;

    for(int i = 0; i < fileListLength; i++) {

        // Get file name length
        int fnameLength = 0;
        if (recv(clientSock, &fnameLength, sizeof(fnameLength), 0) < 0) {
            fatal_error("Error getList (fnameLength): ");
        }

        // DEBUG: fname length
        // std::cout << "fname length: " << fnameLength << std::endl;

        // Get filestring
        recvBuff.resize(fnameLength);
        if (recv(clientSock, recvBuff.data(), fnameLength, 0) < 0) {
            fatal_error("Error getList (fnameLength): ");
        }

        // Pushback string
        fileList.push_back(std::string(recvBuff.begin(), recvBuff.end()));

    }

    return fileList;

}


/* The main function */
int main(int argc, char *argv[]) {
    int clientSock;                 // Socket descriptor
    struct sockaddr_in serv_addr;   // The server address

    std::vector<uint8_t> sendBuff(RCVBUFSIZE);  // Send Buffer
    std::vector<uint8_t> recvBuff(RCVBUFSIZE);  // Recvieve Buffer
    MessageRequest req;             // Message Requestr

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
    // int msglen = studentName.length();
    // if (send(clientSock, studentName.c_str(), msglen, 0) != msglen) {
    //     fatal_error("send() sent an unexpected number of bytes");
    // }

    // Receive the message back from the server
    int returnBytes = 0;      // Initialize number of received bytes
    std::string serverResponse; // String to store the server response

    while (true) {
        int option;
        MessageRequest req;
        std::string res;
        std::vector<std::string> fileList;

        // uint32_t length = htonl(req.requestType.size());
        
        msg_display(option);

        switch(option) {
            case 1:
                req = createMessage(LIST);
                std::cout << "Message Type: " << static_cast<unsigned>(req.type) << std::endl;

                // Load send buffer
                sendBuff[0] = req.type;

                fileList = getList(sendBuff, recvBuff, clientSock);

                for (auto& file : fileList) {
                    std::cout << file << std::endl;
                }
                std::cout << std::endl;
                break;
            case 2:
                // req = createMessage("DIFF");
                // std::cout << req.requestType << std::endl; 
                // send(clientSock, &req, sizeof(req), 0);

                // Check for response from server
                // if(recv(clientSock, rcvBuf, RCVBUFSIZE - 1, 0) < 0) {
                //     std::cout << "Not connected" << std::endl;
                // }
                // std::cout << res << std::endl;
                break;
            case 3:
                // req = createMessage("PULL");
                // std::cout << req.requestType << std::endl;
                // send(clientSock, &req, sizeof(req), 0);
                break;
            case 4:
                // req = createMessage("LEAVE");
                // std::cout << req.requestType << std::endl;
                // send(clientSock, &req, sizeof(req), 0);
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
