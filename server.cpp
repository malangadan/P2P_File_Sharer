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
#include <fstream> 
#include <vector>

namespace fs = std::filesystem;

#define RCVBUFSIZE 512     // The receive buffer size
#define SNDBUFSIZE 512     // The send buffer size
// #define BUFSIZE 40         // Your name can be as many as 40 chars
#define MAXPENDING 10

struct File{
    std::string fileName;
    std::string hash;
};

enum RequestType {
    LIST = 1,
    DIFF = 2,
    PULL = 3,
    LEAVE = 4
};

struct MessageRequest {
    RequestType type;
    std::vector<uint8_t> payload;
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
            std::cout << "Incorrect type: " << static_cast<int>(type) << std::endl;
            break;
    }
}

// Decode Message Request



void fatal_error(const std::string& message) {
    perror(message.c_str());
    exit(1);
}

std::vector<std::string> serverDifference(const std::vector<File> &fileNameHash, const std::vector<std::string> &receivedStrings) {
    // Create a vector to store the file names that are present on the server but not on the client
    std::vector<std::string> difference;
    difference.clear();
    // Iterate over all files on the server
    for (const auto &serverFile : fileNameHash) {
        // Check if the server file's hash is in the list of received hashes
        auto it = std::find(receivedStrings.begin(), receivedStrings.end(), serverFile.hash);

        // If the hash is not found in the client's list, add the file name to the difference vector
        if (it == receivedStrings.end()) {
            difference.push_back(serverFile.fileName);
        }
    }

    // Return the vector of files that are different
    return difference;
}

// Function to compute the hashes of all files in a directory and append them to a given string
void compute_hashes_in_directory(const std::string &directoryPath, std::vector<File> &fileNameHash) {
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

            // Get the file size to decide whether it's small or large
            file.seekg(0, std::ifstream::end);
            std::streamsize fileSize = file.tellg();
            file.seekg(0, std::ifstream::beg); // Reset file pointer to the beginning

            if (fileSize <= 1024) {
                // For files <= 1 KB, read the whole file at once
                char buffer[fileSize];
                file.read(buffer, fileSize);
                if (EVP_DigestUpdate(mdctx, buffer, fileSize) != 1) {
                    std::cerr << "Error: Failed to update SHA-256 hash for small file" << std::endl;
                    EVP_MD_CTX_free(mdctx);
                    continue;
                }
            } else {
                // For larger files, process in chunks (1024 bytes)
                char buffer[1024];
                while (file.read(buffer, sizeof(buffer))) {
                    if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
                        std::cerr << "Error: Failed to update SHA-256 hash for large file" << std::endl;
                        EVP_MD_CTX_free(mdctx);
                        continue;
                    }
                }
            }

            // Finalize the SHA-256 digest
            if (EVP_DigestFinal_ex(mdctx, hash, &length) != 1) {
                std::cerr << "Error: Failed to finalize SHA-256 hash" << std::endl;
                EVP_MD_CTX_free(mdctx);
                continue;
            }

            EVP_MD_CTX_free(mdctx);

            // Convert the hash to a hexadecimal string
            std::string hexHash;
            for (unsigned int i = 0; i < length; i++) {
                char hex[3];
                snprintf(hex, sizeof(hex), "%02x", hash[i]);
                hexHash.append(hex);
            }

            // Create a File object and add it to the vector
            File fileObj;
            fileObj.fileName = entry.path().filename().string();
            fileObj.hash = hexHash;

            // Debug prints for file name and hash
            // std::cout << "File: " << fileObj.fileName << std::endl;
            // std::cout << "Hash: " << fileObj.hash << std::endl;

            fileNameHash.push_back(fileObj);
        }
    }
}

void listFiles(std::vector<std::string> &fileList, const std::string &currentDirectoryPath){
    for (const auto &entry : fs::directory_iterator(currentDirectoryPath)) {
        std::string fname = entry.path().filename().string();
        // std::cout << fname << std::endl;
        fileList.push_back(fname);
    }
}

std::vector<std::string> getHashList(std::vector<uint8_t> &sendBuff, std::vector<uint8_t> &recvBuff, int &clientSock) {
    // Receive the length of the hash list from the client (first, receive the total length of the hash list)
    int hashListLength = 0;
    if (recv(clientSock, &hashListLength, sizeof(hashListLength), 0) < 0) {
        fatal_error("Error receiving hash list length");
    }
    std::cout << "Hash List Length: " << hashListLength << std::endl;

    std::vector<std::string> hashList;

    for (int i = 0; i < hashListLength; ++i) {

        // Receive the length of the hash string
        int hashLength = 0;
        if (recv(clientSock, &hashLength, sizeof(hashLength), 0) < 0) {
            fatal_error("Error receiving hash length");
        }

        // Resize the buffer to hold the hash string
        recvBuff.resize(hashLength);

        // Receive the actual hash string
        if (recv(clientSock, recvBuff.data(), hashLength, 0) < 0) {
            fatal_error("Error receiving hash string");
        }

        // Convert the received buffer into a string and add it to the hashList
        hashList.push_back(std::string(recvBuff.begin(), recvBuff.end()));
    }

    return hashList;
}

std::vector<uint8_t> copyFileToBuffer(std::string filePath) {
        
    // Print file path
    // std::cout << "File path: " << filePath << std::endl;

    // Get contents and store in buffer

    // Open file
    std::ifstream file(filePath, std::ifstream::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
    }
    
    // Get file length
    file.seekg(0, std::ifstream::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ifstream::beg); // Reset file pointer to the beginning

    // std::cout << "File Size: " << fileSize << std::endl;

    std::vector<uint8_t> recvBuffer;
    std::vector<char> fileBuffer(1024);

    // Read to buffer
    while (file) {
        file.read(fileBuffer.data(), fileBuffer.size());
        std::streamsize bytesRead = file.gcount();
        std::cout << "bytes read: " << bytesRead << std::endl;
    
        // append to end of buffer
        recvBuffer.insert(recvBuffer.end(), fileBuffer.begin(), fileBuffer.begin() + bytesRead);
    }

    // std::cout << "Buffer Size: " << recvBuffer.size() << std::endl;


    return recvBuffer;

}

void executeCommand(RequestType &type, std::vector<uint8_t> &sendBuff, const std::string &currentDirectoryPath,
int &clientSock, std::vector<File> &fileNameHash) {
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
            std::cout << fname << std::endl;
        }

    }
     else if ((type == DIFF)) {
        std::cout << "Type: DIFF" << std::endl;
        
        // Generate difference
        int vectorSize = 0;
        if (recv(clientSock, &vectorSize, sizeof(vectorSize), 0) < 0) {
            fatal_error("Error receiving vector size");
        }
        std::cout << "Received vector size: " << vectorSize << std::endl;
        
        std::vector<std::string> receivedStrings;
        receivedStrings.clear();
        for (int i = 0; i < vectorSize; ++i) {
            int strLength = 0;
            if (recv(clientSock, &strLength, sizeof(strLength), 0) < 0) {
                fatal_error("Error receiving string length");
            }

            char buffer[1024];
            if (recv(clientSock, buffer, strLength, 0) < 0) {
                fatal_error("Error receiving string data");
            }
            buffer[strLength] = '\0';  // Null-terminate the string
            receivedStrings.push_back(std::string(buffer));
        }
        fileNameHash.clear();
        compute_hashes_in_directory(currentDirectoryPath,fileNameHash);
        // Print received strings
        std::cout << "Received strings from client:" << std::endl;
        for (const auto &str : receivedStrings) {
            std::cout << str << std::endl;
        }

        std::vector<std::string> difference = serverDifference(fileNameHash,receivedStrings);
        std::cout << "Difference of strings from client:" << std::endl;
        for (const auto &str : difference) {
            std::cout << str << std::endl;
        }
        // Send back the same vector to the client (or modify it if needed)
        int sendVectorSize = difference.size();
        std::cout << "Difference of strings frm client size " << sendVectorSize<< std::endl;

        if (send(clientSock, &sendVectorSize, sizeof(sendVectorSize), 0) < 0) {
            fatal_error("Error sending vector size");
        }

        for (const auto &str : difference) {
            int strLength = str.size();
            if (send(clientSock, &strLength, sizeof(strLength), 0) < 0) {
                fatal_error("Error sending string length");
            }

            if (send(clientSock, str.c_str(), strLength, 0) < 0) {
                fatal_error("Error sending string data");
            }
        }
    }
    else if (type == PULL){
        std::cout << "Type: DIFF" << std::endl;
        // Generate difference
        int vectorSize = 0;
        if (recv(clientSock, &vectorSize, sizeof(vectorSize), 0) < 0) {
            fatal_error("Error receiving vector size");
        }
        std::cout << "Received vector size: " << vectorSize << std::endl;
        
        std::vector<std::string> receivedStrings;
        receivedStrings.clear();
        for (int i = 0; i < vectorSize; ++i) {
            int strLength = 0;
            if (recv(clientSock, &strLength, sizeof(strLength), 0) < 0) {
                fatal_error("Error receiving string length");
            }

            char buffer[1024];
            if (recv(clientSock, buffer, strLength, 0) < 0) {
                fatal_error("Error receiving string data");
            }
            buffer[strLength] = '\0';  // Null-terminate the string
            receivedStrings.push_back(std::string(buffer));
        }
        fileNameHash.clear();
        compute_hashes_in_directory(currentDirectoryPath,fileNameHash);
        // Print received strings
        std::cout << "Received strings from client:" << std::endl;
        for (const auto &str : receivedStrings) {
            std::cout << str << std::endl;
        }

        std::vector<std::string> difference = serverDifference(fileNameHash,receivedStrings);
        std::cout << "Difference of strings from client:" << std::endl;
        for (const auto &str : difference) {
            std::cout << str << std::endl;
        }
        // Send back the same vector to the client (or modify it if needed)
        int sendVectorSize = difference.size();
        std::cout << "Difference of strings frm client size " << sendVectorSize<< std::endl;

        if (send(clientSock, &sendVectorSize, sizeof(sendVectorSize), 0) < 0) {
            fatal_error("Error sending vector size");
        }

        for (const auto &str : difference) {
            int strLength = str.size();
            if (send(clientSock, &strLength, sizeof(strLength), 0) < 0) {
                fatal_error("Error sending string length");
            }

            if (send(clientSock, str.c_str(), strLength, 0) < 0) {
                fatal_error("Error sending string data");
            }
        }
    }

    }
    // else if (nameBuf == "LEAVE"){
    //     nameBuf = "Closing Connection";
    // }
    //  else {
    //     nameBuf = "Unknown command please retry";
    // }


// Thread function for handling client connections
void *handleClient(void *arg) {
    int clientSock = *((int*)arg);
    free(arg);
    std::vector<uint8_t> recvBuffer(RCVBUFSIZE);
    std::vector<uint8_t> sendBuffer(SNDBUFSIZE);
    std::vector<File> fileNameHash;
    std::string currentDirectoryPath;
    std::string targetDirectoryPath;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        targetDirectoryPath = "/ServerStore/";
        currentDirectoryPath = std::string(cwd) + std::string(targetDirectoryPath);
    } else {
        perror("getcwd() error");
    }

    while (true) {
        int recvMsg = recv(clientSock, recvBuffer.data(), RCVBUFSIZE, 0);
        if (recvMsg < 0) {
            perror("recv() failed");
        } else if (recvMsg == 0) {
            std::cout << "Client closed connection" << std::endl;
            close(clientSock);
            pthread_exit(NULL);
        }

        uint8_t reqType = recvBuffer[0];
        RequestType reqTypeEncoded = encodeType(reqType);
        executeCommand(reqTypeEncoded, sendBuffer, currentDirectoryPath,clientSock, fileNameHash);

    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int serverSock;
    struct sockaddr_in changeServAddr;
    struct sockaddr_in changeClntAddr;
    unsigned short changeServPort = 9090;
    unsigned int clntLen;

    if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fatal_error("Socket Failed");
    }

    memset(&changeServAddr, 0, sizeof(changeServAddr));
    changeServAddr.sin_family = AF_INET;
    changeServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    changeServAddr.sin_port = htons(changeServPort);

    if (bind(serverSock, (struct sockaddr *) &changeServAddr, sizeof(changeServAddr)) < 0) {
        fatal_error("bind() failed");
    }

    if (listen(serverSock, MAXPENDING) < 0) {
        fatal_error("listen() failed");
    }

    while (true) {
        clntLen = sizeof(changeClntAddr);
        int *clientSock = (int*)malloc(sizeof(int));  // Allocate memory for clientSock
        if ((*clientSock = accept(serverSock, (struct sockaddr *) &changeClntAddr, &clntLen)) < 0) {
            fatal_error("accept() failed");
        }

        pthread_t threadID;
        if (pthread_create(&threadID, NULL, handleClient, (void*)clientSock) != 0) {
            fatal_error("pthread_create() failed");
        }

        pthread_detach(threadID);  // Ensure resources are released when the thread finishes
    }

    return 0;
}