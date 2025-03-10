/* 
Group Members: Nathan Book and Lindsey Lydon
Class: 4099
Spring Semester 2025
*/

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <vector>
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_PEER_ID 4294967295 // Maximum value for peer ID
#define BUFFER_SIZE 1024 // Maximum buffer size

//void sendRequest(const std::vector<uint8_t>& request, const std::string &host, int server_port);

int main(int argc, char *argv[]) {
    // Validate command-line arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Registry IP/Hostname> <Registry Port> <Peer ID>" << std::endl; 
        return 1;
    }
    
    std::string registry_ip = argv[1];

    int registry_port = std::stoi(argv[2]);
    try {
        registry_port = std::stoi(argv[2]);
        if (registry_port <= 2000 || registry_port > 65535) {
            throw std::out_of_range("Invalid port range");
        }
    } catch (...) {
        std::cerr << "Invalid port number" << std::endl;
        return 1;
    }

    uint32_t peer_id = std::stoul(argv[3]);
    try {
        peer_id = std::stoul(argv[3]);
        if (peer_id > MAX_PEER_ID) {
            throw std::out_of_range("Peer ID too large");
        }
    } catch (...) {
        std::cerr << "Invalid Peer ID. Must be a positive number less than " << MAX_PEER_ID << std::endl;
        return 1;
    }
    
    int sock = -1;  // Socket is initially closed

    std::string command;
    while (true) {
        std::cout << "Enter a command (JOIN, PUBLISH, SEARCH, EXIT): "; 
        //Will tell the user it's invalid if not ALL CAP
        std::cin >> command;

        if (command == "JOIN") {
            if (sock != -1) {
                std::cout << "Already joined. Use EXIT to disconnect first." << std::endl;
                continue;
            }

            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                std::cerr << "Error creating socket" << std::endl;
                return 1;
            }

            //Modeled off of program 1
            struct addrinfo hints = {}; 
            struct addrinfo *result;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(registry_ip.c_str(), std::to_string(registry_port).c_str(), &hints, &result) != 0) {
                std::cerr << "Failed to resolve hostname: " << registry_ip << std::endl;
                close(sock);
                return 1;
            }

            if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
                std::cerr << "Connection to registry failed" << std::endl;
                freeaddrinfo(result);
                close(sock);
                return 1;
            }

            freeaddrinfo(result);

            // Send JOIN request
            std::vector<uint8_t> join_request;
            join_request.push_back(0); // Action = 0 for JOIN
            uint32_t network_peer_id = htonl(peer_id);
            join_request.insert(join_request.end(), reinterpret_cast<uint8_t*>(&network_peer_id), reinterpret_cast<uint8_t*>(&network_peer_id) + sizeof(network_peer_id));

            if (send(sock, join_request.data(), join_request.size(), 0) == -1) {
                std::cerr << "Error sending JOIN request" << std::endl;
                close(sock);
                sock = -1;
                continue;
            }

            // std::cout << "Successfully joined registry as Peer " << peer_id << std::endl; Used for debugging
        } 
        else if (command == "PUBLISH") {
                if (sock == -1) {
                std::cout << "You must JOIN first." << std::endl;
                continue;
            }

            std::vector<std::string> files;
            for (const auto& entry : std::filesystem::directory_iterator("SharedFiles")) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().filename().string());
                }
            }

            uint32_t count = files.size();
            uint32_t networkCount = htonl(count);

            std::vector<uint8_t> request;
            request.push_back(1); // Action = 1 for PUBLISH
            request.insert(request.end(), reinterpret_cast<uint8_t*>(&networkCount), reinterpret_cast<uint8_t*>(&networkCount) + sizeof(networkCount));

            for (const auto& filename : files) {
                request.insert(request.end(), filename.begin(), filename.end());
                request.push_back(0);
            }

            if (send(sock, request.data(), request.size(), 0) == -1) {
                std::cerr << "Error sending PUBLISH request" << std::endl;
                continue;
            }

        } 
        else if (command == "SEARCH") {
            if (sock == -1) {
                std::cout << "You must JOIN first." << std::endl;
                continue;
            }
        
            // Ask user for the filename
            std::string filename;
            std::cout << "Enter a filename: ";
            std::cin >> filename;
        
            // Build SEARCH request
            std::vector<uint8_t> request;
            request.push_back(2); // Action = 2 for SEARCH
            request.insert(request.end(), filename.begin(), filename.end());
            request.push_back(0); // Null terminator
        
            // Send
            if (send(sock, request.data(), request.size(), 0) == -1) {
                std::cerr << "Error sending SEARCH request" << std::endl;
                continue;
            }
        
            // We expect exactly 10 bytes: [PeerID(4B, little-endian)][IP(4B, big-endian)][Port(2B, big-endian)]
            uint8_t resp[10];
            int totalReceived = 0;
        
            // Read in a loop (partial reads)
            while (totalReceived < 10) {
                int n = recv(sock, resp + totalReceived, 10 - totalReceived, 0);
                if (n <= 0) {
                    std::cout << "No results found for file: " << filename << std::endl;
                    break;
                }
                totalReceived += n;
            }
        
            // If we didn’t get all 10 bytes, skip parse
            if (totalReceived < 10) {
                continue;
            }
        
            //We read the first 4 bytes received as the peer_ID
            uint32_t foundPeerId =
                (static_cast<uint32_t>(resp[0]) << 24) |
                (static_cast<uint32_t>(resp[1]) << 16) |
                (static_cast<uint32_t>(resp[2]) <<  8) |
                 static_cast<uint32_t>(resp[3]);
        
            //We read the next 4 bytes received as the IP
            uint32_t netIp =
                (static_cast<uint32_t>(resp[7]) << 24) |
                (static_cast<uint32_t>(resp[6]) << 16) |
                (static_cast<uint32_t>(resp[5]) <<  8) |
                 static_cast<uint32_t>(resp[4]);
        
            // We'll put 'netIp' directly into 'addr.s_addr' since it’s already big-endian
            struct in_addr addr;
            addr.s_addr = netIp;
            char ipStr[INET_ADDRSTRLEN];
            if (!inet_ntop(AF_INET, &addr, ipStr, sizeof(ipStr))) {
                std::cerr << "inet_ntop failed\n";
                continue;
            }
        
            //Finally, last 2 bytes are the port 
            uint16_t foundPort =
                (static_cast<uint16_t>(resp[8]) << 8) |
                 static_cast<uint16_t>(resp[9]);
        
            // If it’s all zero, means "file not found"
            if (foundPeerId == 0 && netIp == 0 && foundPort == 0) {
                std::cout << "File not indexed by registry" << std::endl;
            } else {
                std::cout << "File found at\n";
                std::cout << "Peer " << foundPeerId << "\n";
                std::cout << ipStr << ":" << foundPort << std::endl;
            }
        }         
        else if (command == "EXIT") {
            if (sock != -1) {
                shutdown(sock, SHUT_WR);
                close(sock);
                sock = -1;
            }
            break; // Exit the loop
        } 
        else {
            std::cout << "Invalid command!" << std::endl;
        }
    }
    return 0;
}
