/* 
Group Members: Nathan Book and Lindsey Lydon
Class: 4099
Spring Semester 2025 
*/

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_PEER_ID 4294967295 // Maximum value for peer ID
#define BUFFER_SIZE 1024 // Maximum buffer size

void sendRequest(const std::string &request, const std::string &host, int server_port);

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
    
    std::string command;
    while (true) {
        // Prompt the user for a command
        std::cout << "Enter a command (JOIN, PUBLISH, SEARCH, EXIT): ";
        std::cin >> command;
        
        // Process user input and send appropriate request
        if (command == "JOIN") {
            sendRequest("JOIN " + std::to_string(peer_id), registry_ip, registry_port);
        } else if (command == "PUBLISH") {
            std::string filename;
            std::cout << "Enter a filename: ";
            std::cin >> filename;
            sendRequest("PUBLISH " + filename + " " + std::to_string(peer_id), registry_ip, registry_port);
        } else if (command == "SEARCH") {
            std::string filename;
            std::cout << "Enter a filename: ";
            std::cin >> filename;
            sendRequest("SEARCH " + filename, registry_ip, registry_port);
        } else if (command == "EXIT") {
            break; // Exit the loop and terminate the program
        } else {
            std::cout << "Invalid command!" << std::endl;
        }
    }
    return 0;
}

// Function to send a request to the server and receive a response
void sendRequest(const std::string &request, const std::string &host, int server_port) {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Set up the server address struct
    struct addrinfo hints = {};
	struct addrinfo *result;
    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    // Resolve hostname to IP address
    if (getaddrinfo(host.c_str(), std::to_string(server_port).c_str(), &hints, &result) != 0) {
        std::cerr << "Failed to resolve hostname: " << host << std::endl;
        close(sock);
        return;
     }

    // Connect to the server
    if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
        std::cerr << "Connection failed" << std::endl;
        freeaddrinfo(result);
        close(sock);
        return;
    }

    freeaddrinfo(result); // Free address info after successful connection

    // Send the request to the server
    if (send(sock, request.c_str(), request.length(), 0) == -1) {
        std::cerr << "Error sending request" << std::endl;
        close(sock);
        return;
    }
    
    // Buffer to store the response from the server
    char buffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate the received data
         
        // Check if response contains file information
         std::string response(buffer);
         if (response.find("FOUND") != std::string::npos) {
             std::istringstream ss(response);
             std::string status, peerID, ipPort;
             ss >> status >> peerID >> ipPort;
 
             std::cout << "File found at" << std::endl;
             std::cout << "Peer " << peerID << std::endl;
             std::cout << ipPort << std::endl;
         } else {
             std::cout << "File not indexed by registry" << std::endl;
         }
    } else if (bytesReceived == 0) {
        std::cerr << "Server closed the connection" << std::endl;
    } else {
        std::cerr << "Error receiving response" << std::endl;
    }

    // Close the socket
    shutdown(sock, SHUT_WR);
    close(sock);
}