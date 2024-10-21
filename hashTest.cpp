#include <iostream>
#include <string>
#include <unordered_set>
#include <sstream>


struct{

}


// Function to add hashes from a string (delimited by newlines) into a set
void addHashesToSet(const std::string &input, std::unordered_set<std::string> &hashSet) {
    std::string hash;
    std::istringstream stream(input);
    while (std::getline(stream, hash)) {
        hashSet.insert(hash);
    }
}

// Function to compare two strings of hashes (delimited by new lines) and return unique hashes
std::string getUniqueHashes(const std::string &clientHashes, const std::string &serverHashes) {
    // Sets to store the client and server hashes
    std::unordered_set<std::string> clientSet;
    std::unordered_set<std::string> serverSet;
    
    // Add hashes from each string to their respective sets
    addHashesToSet(clientHashes, clientSet);
    addHashesToSet(serverHashes, serverSet);

    // Set to hold unique hashes (in either client or server but not both)
    std::unordered_set<std::string> uniqueHashes;
    
    // Find unique hashes in client set (not in server set)
    for (const auto &hash : clientSet) {
        if (serverSet.find(hash) == serverSet.end()) {
            uniqueHashes.insert(hash);  // Hash is only in client, not in server
        }
    }
    
    // Find unique hashes in server set (not in client set)
    for (const auto &hash : serverSet) {
        if (clientSet.find(hash) == clientSet.end()) {
            uniqueHashes.insert(hash);  // Hash is only in server, not in client
        }
    }

    // Convert the unique hashes to a string separated by new lines
    std::string result;
    for (const auto &hash : uniqueHashes) {
        result += hash + "\n";
    }
    
    return result;
}

int main() {
    // Example client and server hashes
    std::string clientHashes = "hash1\nhash2\nhash3\n";
    std::string serverHashes = "hash2\nhash3\nhash4\n";
    
    // Get unique hashes
    std::string uniqueHashes = getUniqueHashes(clientHashes, serverHashes);
    
    // Print the result
    std::cout << "Unique hashes:\n" << uniqueHashes;
    
    return 0;
}
