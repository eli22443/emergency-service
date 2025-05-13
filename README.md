# SPL3: STOMP Messaging System

## Overview
This project implements a client-server messaging system using the STOMP protocol. The server is written in Java and the client in C++. The system allows users to connect, join channels, report events, and generate summaries.

## Project Structure

- `server/` - Java server implementation (Maven project)
- `client/` - C++ client implementation
- `.vscode/`, `.devcontainer/` - Development environment configuration

## Prerequisites

### Server (Java)
- Java 8+
- Maven

### Client (C++)
- g++ with C++11 support
- Boost libraries (for `-lboost_system`)
- pthreads

## Build Instructions

### Server
1. Navigate to the `server` directory:
   ```bash
   cd server
   ```
2. Build the server using Maven:
   ```bash
   mvn package
   ```
   This will generate a JAR file in the `target/` directory.

### Client
1. Navigate to the `client` directory:
   ```bash
   cd client
   ```
2. Build the client using the provided makefile:
   ```bash
   make
   ```
   This will produce the `bin/StompEMIClient` executable.

## Running the Applications

### Server
The server can be run in two modes: `tpc` (thread-per-client) or `reactor` (reactor pattern).

Usage:
```bash
java -cp target/server-1.0.jar bgu.spl.net.impl.stomp.StompServer <port> <tpc|reactor>
```
Example:
```bash
java -cp target/server-1.0.jar bgu.spl.net.impl.stomp.StompServer 7777 tpc
```

### Client
The main client executable is `bin/StompEMIClient`.

Usage:
```bash
./bin/StompEMIClient
```
This will start the client and allow you to enter commands interactively.

#### Supported Commands
- `login {host:port} {username} {password}`: Connect and log in to the server.
- `join {channel_name}`: Subscribe to a channel.
- `exit {channel_name}`: Unsubscribe from a channel.
- `report {file_name}`: Report events from a JSON file.
- `summary {file_name} {user}`: Generate a summary for a user and write to a file.
- `logout`: Disconnect from the server.

#### Example Session
```
login localhost:7777 alice password123
join /sports
report events.json
summary summary.txt alice
logout
```

### Echo Client (for testing)
You can also run the echo client for basic connectivity testing:
```bash
./bin/echoClient <host> <port>
```

## Event File Format
Event files should be in JSON format. See the assignment instructions for details.

## License
This project is for educational purposes. 