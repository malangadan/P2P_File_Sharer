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

// Function to compute the hashes of all files in a directory and append them to a given string
void compute_hashes_in_directory(const std::string &directoryPath, std::vector<std::string> &hashList) {
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
            hashList.push_back(hexHash);
        }
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

std::vector<std::string> getDiff(std::vector<uint8_t> &sendBuff, std::vector<uint8_t> &recvBuff, int &clientSock, std::vector<std::string> &hashList) {
    // Send message
    send(clientSock, sendBuff.data(), sendBuff.size(), 0);

    // Send hashList
    int length = hashList.size();
    if (send(clientSock, &length, sizeof(length), 0) < 0) {
        fatal_error("List Length Error: ");
    }

    // Send each hash string
    for (auto &hash : hashList) {
        int hashLength = hash.size();
        if (send(clientSock, &hashLength, sizeof(hashLength), 0) < 0) {
            fatal_error("List Error (hashLength): ");
        }

        if (send(clientSock, hash.c_str(), hashLength, 0) < 0) {
            fatal_error("List Error (hash): ");
        }
    }
    
    // Get response
    int diffListLength = 0;
    if (recv(clientSock, &diffListLength, sizeof(diffListLength), 0) < 0) {
        fatal_error("Receive Error (diffListLength): ");
    }

    std::cout << "Number of different files: " << diffListLength << std::endl;

    std::vector<std::string> diffList;

    // Receive each file's name or hash in the diff list
    for (int i = 0; i < diffListLength; i++) {
        // Receive file name length
        int fileNameLength = 0;
        if (recv(clientSock, &fileNameLength, sizeof(fileNameLength), 0) < 0) {
            fatal_error("Receive Error (fileNameLength): ");
        }

        // Resize buffer and receive the file name
        recvBuff.resize(fileNameLength);
        if (recv(clientSock, recvBuff.data(), fileNameLength, 0) < 0) {
            fatal_error("Receive Error (fileName): ");
        }

        // Convert buffer to string and add to diffList
        diffList.push_back(std::string(recvBuff.begin(), recvBuff.end()));
    }

    return diffList;
   
}





/* The main function */
int main(int argc, char *argv[]) {

    // Get path into string
    std::string currentDirectoryPath;
    std::vector<std::string> hashList;
    char cwd[PATH_MAX];    // Store current path
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currentDirectoryPath = std::string(cwd);
        std::cout << "Current directory: " << currentDirectoryPath << std::endl;
    } else {
        perror("getcwd() error");
    }
    // Get hash of all files
    compute_hashes_in_directory(currentDirectoryPath,hashList);
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
                req = createMessage(DIFF);
                std::cout << "Message Type: " << static_cast<unsigned>(req.type) << std::endl;

                // Load send buffer
                sendBuff[0] = req.type;

                fileList = getDiff(sendBuff, recvBuff, clientSock,hashList);

                for (auto& file : fileList) {
                    std::cout << file << std::endl;
                }
                std::cout << std::endl;
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