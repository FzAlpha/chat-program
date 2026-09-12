#include "common.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atomic>

struct Client {
    int id;
    std::string name;
    int socketFd;
    std::string ip;
    int port;
};

// Global server state
std::atomic<bool> serverRunning{true};
int serverSocketFd = -1;
std::vector<Client> clients;
std::mutex clientsMutex;
std::atomic<int> clientIdCounter{1};

void broadcastMessage(const std::string& message, int senderId = -1) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& client : clients) {
        if (client.id != senderId) {
            send(client.socketFd, message.c_str(), message.length(), 0);
        }
    }
}

void sendDirectMessage(int targetSocketFd, const std::string& message) {
    send(targetSocketFd, message.c_str(), message.length(), 0);
}

void signalHandler(int signum) {
    (void)signum;
    serverRunning = false;
    std::cout << "\n" << Chat::Color::YELLOW 
              << "[" << Chat::getTimestamp() << "] [Server] Shutting down server..." 
              << Chat::Color::RESET << std::endl;

    std::string shutdownMsg = "\n" + Chat::Color::RED + Chat::Color::BOLD + 
                             "[" + Chat::getTimestamp() + "] [Server] Server is shutting down. Disconnected." + 
                             Chat::Color::RESET + "\n";
    broadcastMessage(shutdownMsg);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto& client : clients) {
            close(client.socketFd);
        }
        clients.clear();
    }

    if (serverSocketFd != -1) {
        close(serverSocketFd);
        serverSocketFd = -1;
    }
    _exit(0);
}

void handleClient(Client client) {
    char buffer[Chat::BUFFER_SIZE];

    // First receive the username
    int bytesReceived = recv(client.socketFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        close(client.socketFd);
        return;
    }

    buffer[bytesReceived] = '\0';
    client.name = Chat::trim(std::string(buffer));
    if (client.name.empty()) {
        client.name = "Anonymous_" + std::to_string(client.id);
    }

    // Register client in list
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back(client);
    }

    // Server log
    std::cout << Chat::Color::GREEN << "[" << Chat::getTimestamp() << "] [JOIN] " 
              << Chat::Color::BOLD << client.name << Chat::Color::RESET 
              << " (" << client.ip << ":" << client.port << ") connected. Active clients: " 
              << clients.size() << std::endl;

    // Broadcast join notification to others
    std::string joinMsg = Chat::Color::YELLOW + "[" + Chat::getTimestamp() + "] [Server] " +
                          Chat::Color::BOLD + client.name + Chat::Color::RESET + 
                          Chat::Color::YELLOW + " has entered the chat." + Chat::Color::RESET + "\n";
    broadcastMessage(joinMsg, client.id);

    // Send welcome message to the newly joined client
    std::string welcomeMsg = Chat::Color::CYAN + Chat::Color::BOLD + 
                             "=================================================\n" +
                             " Welcome to the C++ Terminal Chat, " + client.name + "!\n" +
                             " Commands: /list (show active users), /quit (leave)\n" +
                             "=================================================\n" +
                             Chat::Color::RESET;
    sendDirectMessage(client.socketFd, welcomeMsg);

    // Message loop
    while (serverRunning) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(client.socketFd, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            // Client disconnected or error
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string rawMsg = Chat::trim(std::string(buffer));

        if (rawMsg.empty()) {
            continue;
        }

        // Handle client commands
        if (rawMsg == "/list") {
            std::string userList = Chat::Color::CYAN + "[" + Chat::getTimestamp() + "] [Online Users]: ";
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (size_t i = 0; i < clients.size(); ++i) {
                    userList += (clients[i].id == client.id ? (clients[i].name + " (You)") : clients[i].name);
                    if (i + 1 < clients.size()) userList += ", ";
                }
            }
            userList += Chat::Color::RESET + "\n";
            sendDirectMessage(client.socketFd, userList);
            continue;
        }

        if (rawMsg == "/quit" || rawMsg == "/exit") {
            break;
        }

        // Server message logging
        std::cout << Chat::Color::GRAY << "[" << Chat::getTimestamp() << "] " 
                  << Chat::Color::BOLD << client.name << Chat::Color::RESET 
                  << ": " << rawMsg << std::endl;

        // Broadcast to other clients
        std::string formattedMsg = Chat::Color::GRAY + "[" + Chat::getTimestamp() + "] " +
                                   Chat::Color::CYAN + Chat::Color::BOLD + client.name + 
                                   Chat::Color::RESET + ": " + rawMsg + "\n";
        broadcastMessage(formattedMsg, client.id);
    }

    // Client leaving cleanup
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [&](const Client& c) { return c.id == client.id; }),
            clients.end()
        );
    }

    close(client.socketFd);

    // Server log
    std::cout << Chat::Color::RED << "[" << Chat::getTimestamp() << "] [LEAVE] " 
              << Chat::Color::BOLD << client.name << Chat::Color::RESET 
              << " disconnected. Active clients: " << clients.size() << std::endl;

    // Broadcast leave notification to remaining clients
    std::string leaveMsg = Chat::Color::YELLOW + "[" + Chat::getTimestamp() + "] [Server] " +
                           Chat::Color::BOLD + client.name + Chat::Color::RESET + 
                           Chat::Color::YELLOW + " has left the chat." + Chat::Color::RESET + "\n";
    broadcastMessage(leaveMsg, client.id);
}

int main(int argc, char* argv[]) {
    int port = Chat::DEFAULT_PORT;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port provided. Using default " << Chat::DEFAULT_PORT << std::endl;
            port = Chat::DEFAULT_PORT;
        }
    }

    // Register signal handler for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create server socket
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0) {
        std::cerr << Chat::Color::RED << "Error creating socket!" << Chat::Color::RESET << std::endl;
        return 1;
    }

    // Allow immediate socket reuse after server restart
    int opt = 1;
    if (setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << Chat::Color::RED << "Error setting socket options!" << Chat::Color::RESET << std::endl;
        close(serverSocketFd);
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << Chat::Color::RED << "Failed to bind to port " << port << ". Check if port is already in use." << Chat::Color::RESET << std::endl;
        close(serverSocketFd);
        return 1;
    }

    if (listen(serverSocketFd, 10) < 0) {
        std::cerr << Chat::Color::RED << "Error listening on socket!" << Chat::Color::RESET << std::endl;
        close(serverSocketFd);
        return 1;
    }

    std::cout << Chat::Color::GREEN << Chat::Color::BOLD;
    std::cout << "========================================================\n";
    std::cout << "        🚀 C++ Terminal Chat Server Started 🚀          \n";
    std::cout << "========================================================\n" << Chat::Color::RESET;
    std::cout << Chat::Color::CYAN << "Listening on port: " << Chat::Color::BOLD << port << Chat::Color::RESET << "\n";
    std::cout << Chat::Color::GRAY << "Press Ctrl+C to stop server at any time.\n" << Chat::Color::RESET;
    std::cout << "--------------------------------------------------------\n";

    while (serverRunning) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverSocketFd, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientFd < 0) {
            if (!serverRunning) break;
            std::cerr << Chat::Color::RED << "Error accepting connection!" << Chat::Color::RESET << std::endl;
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        Client newClient;
        newClient.id = clientIdCounter++;
        newClient.socketFd = clientFd;
        newClient.ip = std::string(ipStr);
        newClient.port = clientPort;

        // Detach client handling thread to handle multiple clients concurrently
        std::thread clientThread(handleClient, newClient);
        clientThread.detach();
    }

    if (serverSocketFd != -1) {
        close(serverSocketFd);
    }
    return 0;
}
