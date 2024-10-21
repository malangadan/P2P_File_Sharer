/*///////////////////////////////////////////////////////////
*
* FILE:        client.cpp
* AUTHOR:      
* PROJECT:     CNT 4007 Project 2 - Professor Traynor
* DESCRIPTION: Network Client Code (C++)
*
*////////////////////////////////////////////////////////////

#include <iostream>        // For input/output in C++
#include <sys/socket.h>    // For socket(), connect(), send(), and recv()
#include <arpa/inet.h>     // For sockaddr_in and inet_addr()
#include <cstdlib>         // For general-purpose functions like exit()
#include <unistd.h>        // For close()
#include <cstring>         // For string manipulation functions
#include <openssl/evp.h>   // For OpenSSL EVP digest libraries/SHA256
#include <string>          // For std::string
#include <algorithm>
#include <fstream> 
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

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

// Function to compute the hashes of all files in a directory and append them to a given string
void compute_hashes_in_directory(const std::string &directoryPath, std::string &hashes) {
    // Iterate over all files in directory
    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry.path())) {
            std::string filePath = entry.path().string();
            unsigned char hash[EVP_MAX_MD_SIZE];  // Buffer to store the hash
            unsigned int length = 0;

            // Initialize the OpenSSL for SHA-256
            EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
            if (mdctx == NULL) {
                std::cerr << "Error: Failed to create EVP_MD_CTX" << std::endl;
                continue;
            }

            // Initialize SHA-256
            if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
                std::cerr << "Error: Failed to initialize SHA-256" << std::endl;
                EVP_MD_CTX_free(mdctx);
                continue;
            }

            // Open the file for reading
            std::ifstream file(filePath, std::ifstream::binary);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file " << filePath << std::endl;
                EVP_MD_CTX_free(mdctx);
                continue;
            }

            // Read the file and update the hash context
            char buffer[1024];
            while (file.read(buffer, sizeof(buffer))) {
                if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
                    std::cerr << "Error: Failed to update SHA-256 hash" << std::endl;
                    EVP_MD_CTX_free(mdctx);
                    continue;
                }
            }

            // Finalize the SHA-256 digest
            if (EVP_DigestFinal_ex(mdctx, hash, &length) != 1) {
                std::cerr << "Error: Failed to finalize SHA-256 hash" << std::endl;
                EVP_MD_CTX_free(mdctx);
                continue;
            }

            EVP_MD_CTX_free(mdctx);

            // Convert the hash to a hexadecimal string and append it to the hashes string
            std::string hexHash;
            for (unsigned int i = 0; i < length; i++) {
                char hex[3];
                snprintf(hex, sizeof(hex), "%02x", hash[i]);
                hexHash.append(hex);
            }

            // Append the computed hash followed by a newline
            hashes += hexHash + "\n";
        }
    }
}

/* The main function */
int main(int argc, char *argv[]) {
    int clientSock;                 // Socket descriptor
    struct sockaddr_in serv_addr;   // The server address

    std::string studentName;        // Your Name
    std::string hashList;
    char rcvBuf[RCVBUFSIZE];        // Receive Buffer
    
    memset(rcvBuf, 0, RCVBUFSIZE);  // Clear receive buffer

    std::string currentDirectoryPath;
    char cwd[PATH_MAX];    // Store current path
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currentDirectoryPath = std::string(cwd);
        std::cout << "Current directory: " << currentDirectoryPath << std::endl;
    } else {
        perror("getcwd() error");
    }
    compute_hashes_in_directory(currentDirectoryPath,hashList);
    std::cout << "Hashes: " << hashList <<currentDirectoryPath << std::endl;

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