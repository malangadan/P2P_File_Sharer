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
#include <filesystem>
namespace fs = std::filesystem;

#define RCVBUFSIZE 512     // The receive buffer size
#define SNDBUFSIZE 512     // The send buffer size
// #define BUFSIZE 40         // Your name can be as many as 40 chars
#define MAXPENDING 10

// struct MessageRequest {
//     // std::vector<std::string> payLoad;
//     std::string requestType;
// };

enum RequestType {
    LIST = 1,
    DIFF = 2,
    PULL = 3,
    LEAVE = 4
};

RequestType encodeType(uint8_t type) {
    switch(type) {
        case 1:
            return LIST;
        case 2:
            return DIFF;
        case 3:
            return PULL;
        case 4:
            return LEAVE;
        default:
            std::cout << "Incorrect type" << std::endl;
            exit(1);
            break;
    }
}


void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}

void listFiles(std::vector<std::string> &fileList, const std::string &currentDirectoryPath){
    for (const auto &entry : fs::directory_iterator(currentDirectoryPath)) {
        std::string fname = entry.path().filename().string();
        // std::cout << fname << std::endl;
        fileList.push_back(fname);
    }
}

void executeCommand(RequestType &type, std::vector<uint8_t> &sendBuff, const std::string &currentDirectoryPath,std::string &fileList,
int &clientSock) {
    std::cout << type << std::endl;
    if (type == LIST) { 
        std::cout << "Type: LIST" << std::endl;
        
        std::vector<std::string> fileList;
        listFiles(fileList, currentDirectoryPath);
        
        int length = fileList.size();
        std::cout << "fileList Size: " << length << std::endl;
        if (send(clientSock, &length, sizeof(length), 0) < 0) {
            fatal_error("List Length Error: ");
        }

        // Send list
        for(auto &fname : fileList) {  
            
            // Send String Length

            int fnameLength = fname.size();
            if (send(clientSock, &fnameLength, sizeof(fnameLength), 0) < 0) {
                fatal_error("List Error (fnameLen): ");
            }

            // Send String
            if (send(clientSock, fname.c_str(), fnameLength, 0) < 0) {
                fatal_error("List Error (fname): ");
            }
        }

    }
    // else if ((nameBuf == "DIFF")) {
    //     nameBuf = "Here are the files the server has and you do not:";
    // }
    // else if (nameBuf == "PULL"){
    //     nameBuf = "Retrieving Files";
    // }
    // else if (nameBuf == "LEAVE"){
    //     nameBuf = "Closing Connection";
    // }
    //  else {
    //     nameBuf = "Unknown command please retry";
    // }
}

int main(int argc, char *argv[]) {
    int serverSock;                // Server Socket
    int clientSock;                // Client Socket
    struct sockaddr_in changeServAddr; // Local address
    struct sockaddr_in changeClntAddr; // Client address
    unsigned short changeServPort = 9090; // Server port (defined as 9090)
    unsigned int clntLen;          // Length of address data struct

    // DEFINE BUFFERS
    std::vector<uint8_t> recvBuffer(RCVBUFSIZE);
    std::vector<uint8_t> sendBuffer(RCVBUFSIZE);

    std::string fileList = "";
    std::string res;   // Buffer to store name from client
    std::string currentDirectoryPath;
    char cwd[PATH_MAX];    // Store current path
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currentDirectoryPath = std::string(cwd);
        std::cout << "Current directory: " << currentDirectoryPath << std::endl;
    } else {
        perror("getcwd() error");
    }

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

        // Recieve Request
        int recvMsg = recv(clientSock, recvBuffer.data(), RCVBUFSIZE, 0);
        if(recvMsg < 0) {
            fatal_error("recv() failed");
        }

        // Decode File
        uint8_t reqType = recvBuffer[0];
        // std::cout << static_cast<unsigned>(reqType) << std::endl;

        RequestType reqTypeEncoded = encodeType(reqType);
        // std::cout << reqTypeEncoded << std::endl;


        // Execute Command
        executeCommand(reqTypeEncoded, sendBuffer, currentDirectoryPath, fileList, clientSock); // Execute user specified action
        std::cout << "execution done" << std::endl;

        close(clientSock);  // Close client connection after handling
    }

    return 0;
}
