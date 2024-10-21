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
#define BUFSIZE 40         // Your name can be as many as 40 chars
#define MAXPENDING 10

void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}

void listFiles(const std::string &currentDirectoryPath, std::string &fileList){
    for (const auto &entry : fs::directory_iterator(currentDirectoryPath)) {
        fileList += entry.path().filename().string() + "\n";  // Add each file name to the string
    }
}


void executeCommand(std::string &nameBuf,const std::string &currentDirectoryPath,std::string &fileList,
int &clientSock) {
    //Eliminate whitespace
    nameBuf.erase(std::remove_if(nameBuf.begin(), nameBuf.end(), ::isspace), nameBuf.end());

    if (nameBuf == "LIST") { 
        nameBuf = "Here is a list of the available music:\n";
        listFiles(currentDirectoryPath,fileList);
        std::string messageToClient = fileList;
        // Send the list of files to the client
        if (send(clientSock, messageToClient.c_str(), messageToClient.size(), 0) != messageToClient.size()) {
            fatal_error("Send failed");
        }
    }
    else if ((nameBuf == "DIFF")) {
        nameBuf = "Here are the files the server has and you do not:";
    }
    else if (nameBuf == "PULL"){
        nameBuf = "Retrieving Files";
    }
    else if (nameBuf == "LEAVE"){
        nameBuf = "Closing Connection";
    }
     else {
        nameBuf = "Unknown command please retry";
    }
}

int main(int argc, char *argv[]) {
    int serverSock;                // Server Socket
    int clientSock;                // Client Socket
    struct sockaddr_in changeServAddr; // Local address
    struct sockaddr_in changeClntAddr; // Client address
    unsigned short changeServPort = 9090; // Server port (defined as 9090)
    unsigned int clntLen;          // Length of address data struct

    std::string fileList = "";
    std::string nameBuf;   // Buffer to store name from client
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
        std::string nameBuf(BUFSIZE, '\0');
        // Extract Your Name from the packet, store in nameBuf
        int recvLen = recv(clientSock, &nameBuf[0], BUFSIZE - 1, 0);
        if (recvLen < 0) {
            fatal_error("recv() failed");
        }

        nameBuf.resize(recvLen);  // Resize the string to fit the actual data received

        nameBuf[recvLen] = '\0';  // Null-terminate the received string
        executeCommand(nameBuf,currentDirectoryPath,fileList,clientSock); // Execute user specified action
        std::cout << "execution done" << std::endl;

        // Send the same nameBuf string back to the client
        if (send(clientSock, nameBuf.c_str(), nameBuf.size(), 0) != nameBuf.size()) {
            fatal_error("Send failed");
        }

        close(clientSock);  // Close client connection after handling
    }

    return 0;
}