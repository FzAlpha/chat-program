# 💬 Terminal-to-Terminal C++ Chat Program

A lightweight, robust, real-time terminal chat application written in modern C++ (C++17) using POSIX TCP Sockets and multithreading.

---

## ✨ Features

- **Real-Time Communication**: Messages are instantly broadcast to all connected terminal clients.
- **Multithreaded I/O**: Separate background thread for receiving incoming messages so incoming text doesn't block the user from typing.
- **Colorized Terminal Output**: ANSI color codes for sender usernames, timestamps (`[HH:MM:SS]`), system announcements, and errors.
- **Commands Support**:
  - `/list` — Display all currently online users.
  - `/quit` or `/exit` — Leave the chat room gracefully.
  - `/help` — Display available commands.
- **Graceful Shutdown**: Proper signal handling (`Ctrl+C` / `SIGINT`) cleans up client connections and frees sockets immediately (`SO_REUSEADDR`).

---

## 📁 Project Structure

```
.
├── src/
│   ├── common.hpp    # Shared constants, ANSI colors, and time utilities
│   ├── server.cpp    # Multithreaded TCP chat server
│   └── client.cpp    # Interactive terminal chat client
├── Makefile          # Makefile for building binaries
├── CMakeLists.txt    # CMake configuration
└── README.md         # Documentation
```

---

## 🛠️ Build Instructions

### Using `make` (Recommended)

To compile both server and client:
```bash
make
```

Binaries will be generated inside the `bin/` directory:
- `bin/server`
- `bin/client`

To clean build artifacts:
```bash
make clean
```

### Using `cmake`

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 🚀 How to Run

### Step 1: Start the Chat Server (Terminal 1)

In your first terminal window, start the server:

```bash
./bin/server [PORT]
```

*Example (Default port 8080):*
```bash
./bin/server 8080
```

---

### Step 2: Connect Clients (Terminal 2, Terminal 3, etc.)

Open another terminal window (or multiple terminals) and connect as a client:

```bash
./bin/client [SERVER_IP] [PORT] [USERNAME]
```

#### Examples:

**In Terminal 2 (Alice):**
```bash
./bin/client 127.0.0.1 8080 Alice
```

**In Terminal 3 (Bob):**
```bash
./bin/client 127.0.0.1 8080 Bob
```

*Note: If you run `./bin/client` without arguments, it will interactively prompt you for your username and connect to `127.0.0.1:8080` by default.*

---

## 🌐 Chatting Across Different Machines on the Same Network (LAN)

1. Find the Server machine's local IP address (e.g. `ip a` or `hostname -I`). Example: `192.168.1.50`.
2. Start the server on the host machine:
   ```bash
   ./bin/server 8080
   ```
3. On any other machine connected to the same WiFi / LAN:
   ```bash
   ./bin/client 192.168.1.50 8080 YourName
   ```

---

## ⌨️ In-Chat Commands

| Command | Description |
| :--- | :--- |
| `/list` | Shows list of active users in the chat room |
| `/help` | Displays available chat commands |
| `/quit` or `/exit` | Safely leaves the chat session |