CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
SRC_DIR = src
BIN_DIR = bin

SERVER_TARGET = $(BIN_DIR)/server
CLIENT_TARGET = $(BIN_DIR)/client

.PHONY: all clean server client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER_TARGET): $(SRC_DIR)/server.cpp $(SRC_DIR)/common.hpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/server.cpp -o $(SERVER_TARGET)

$(CLIENT_TARGET): $(SRC_DIR)/client.cpp $(SRC_DIR)/common.hpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/client.cpp -o $(CLIENT_TARGET)

server: $(SERVER_TARGET)

client: $(CLIENT_TARGET)

clean:
	rm -rf $(BIN_DIR)
