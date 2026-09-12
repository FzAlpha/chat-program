#include "common.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::atomic<bool> clientRunning{true};
int clientSocketFd = -1;
std::string username;

void signalHandler(int signum) {
    (void)signum;
    clientRunning = false;
    std::cout << "\n" << Chat::Color::YELLOW 
              << "[" << Chat::getTimestamp() << "] [Client] Leaving chat..." 
              << Chat::Color::RESET << std::endl;

    if (clientSocketFd != -1) {
        std::string exitMsg = "/quit";
        send(clientSocketFd, exitMsg.c_str(), exitMsg.length(), 0);
        close(clientSocketFd);
        clientSocketFd = -1;
    }
    _exit(0);
}

void receiveMessages() {
    char buffer[Chat::BUFFER_SIZE];

    while (clientRunning) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocketFd, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            if (clientRunning) {
                std::cout << "\n" << Chat::Color::RED << Chat::Color::BOLD 
                          << "[" << Chat::getTimestamp() << "] [Error] Disconnected from chat server." 
                          << Chat::Color::RESET << "\n";
                clientRunning = false;
            }
            break;
        }

        buffer[bytesReceived] = '\0';
        
        // Print message and reprint user prompt
        std::cout << "\r" << buffer;
        if (clientRunning) {
            std::cout << Chat::Color::GREEN << Chat::Color::BOLD << "[You]: " << Chat::Color::RESET << std::flush;
        }
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = Chat::DEFAULT_PORT;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << Chat::Color::YELLOW << "Invalid port provided. Using default " << Chat::DEFAULT_PORT << Chat::Color::RESET << std::endl;
            port = Chat::DEFAULT_PORT;
        }
    }
    if (argc > 3) {
        username = argv[3];
    }

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::cout << Chat::Color::GREEN << Chat::Color::BOLD;
    std::cout << "========================================================\n";
    std::cout << "         💬 C++ Terminal Chat Client 💬                 \n";
    std::cout << "========================================================\n" << Chat::Color::RESET;

    // Interactive prompt for username if not passed
    while (username.empty()) {
        std::cout << Chat::Color::CYAN << "Enter your username: " << Chat::Color::RESET;
        if (!std::getline(std::cin, username)) {
            return 0;
        }
        username = Chat::trim(username);
        if (username.empty()) {
            std::cout << Chat::Color::RED << "Username cannot be empty!\n" << Chat::Color::RESET;
        }
    }

    // Create client socket
    clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd < 0) {
        std::cerr << Chat::Color::RED << "Error creating socket!" << Chat::Color::RESET << std::endl;
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << Chat::Color::RED << "Invalid server address: " << host << Chat::Color::RESET << std::endl;
        close(clientSocketFd);
        return 1;
    }

    std::cout << Chat::Color::GRAY << "Connecting to " << host << ":" << port << "..." << Chat::Color::RESET << std::endl;

    if (connect(clientSocketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << Chat::Color::RED << "Connection failed! Ensure the chat server is running on " 
                  << host << ":" << port << Chat::Color::RESET << std::endl;
        close(clientSocketFd);
        return 1;
    }

    // Send username to server as the first message
    send(clientSocketFd, username.c_str(), username.length(), 0);

    std::cout << Chat::Color::GREEN << "Connected successfully!\n" << Chat::Color::RESET;
    std::cout << Chat::Color::GRAY << "Type your message and press [Enter]. Type /help for options.\n" << Chat::Color::RESET;
    std::cout << "--------------------------------------------------------\n";

    // Start background receiver thread
    std::thread receiverThread(receiveMessages);

    // Main sender loop
    std::string input;
    while (clientRunning) {
        std::cout << Chat::Color::GREEN << Chat::Color::BOLD << "[You]: " << Chat::Color::RESET << std::flush;
        if (!std::getline(std::cin, input)) {
            break;
        }

        input = Chat::trim(input);
        if (input.empty()) {
            continue;
        }

        if (input == "/help") {
            std::cout << Chat::Color::CYAN << Chat::Color::BOLD << "\n--- Chat Commands ---\n" << Chat::Color::RESET
                      << "  /list       - Show all online users in the chat room\n"
                      << "  /quit, /exit - Disconnect and leave the chat\n"
                      << "  /help       - Show this help message\n"
                      << "-----------------------\n";
            continue;
        }

        if (input == "/quit" || input == "/exit") {
            send(clientSocketFd, input.c_str(), input.length(), 0);
            clientRunning = false;
            break;
        }

        // Send message to server
        int bytesSent = send(clientSocketFd, input.c_str(), input.length(), 0);
        if (bytesSent < 0) {
            std::cerr << Chat::Color::RED << "Failed to send message." << Chat::Color::RESET << std::endl;
            break;
        }
    }

    clientRunning = false;
    if (clientSocketFd != -1) {
        close(clientSocketFd);
        clientSocketFd = -1;
    }

    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    std::cout << Chat::Color::YELLOW << "\nDisconnected from chat. Goodbye!\n" << Chat::Color::RESET;
    return 0;
}
