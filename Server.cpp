#include "Server.hpp"

Server::Server(int port) : _server_fd(-1), _port(port) {
    if (port <= 0 || port > 65535) {
        throw(std::runtime_error("Invalid port number"));
    }
}
Server::~Server() {
    // Close all client connections
    for (std::map<int, ClientBuffer>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
    }
    // Close server socket
    if (_server_fd != -1) {
        close(_server_fd);
    }
}

void Server::initserver(){
    // Creating the server socket, IPv4, TCP.
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        throw(std::runtime_error("Socket creation failed"));
    }
    // Setting socket options to allow reuse of the address.
    // _server_fd, socket level, option name, option value(1 = on).
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw(std::runtime_error("Set socket options failed"));
    }
    // Making the socket non-blocking.
    // _server_fd, which flags, option to change.
    if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) < 0) {
        throw(std::runtime_error("Set socket non-blocking failed"));
    }
    // Defining the server address structure.
    struct sockaddr_in address;
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address.
    address.sin_port = htons(_port); // Port number in network byte order.
    // Binding the socket to the specified IP address and port.
    if (bind(_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw(std::runtime_error("Bind failed"));
    }
    // Start listening for incoming connections.
    if (listen(_server_fd, SOMAXCONN) < 0) { // SOMAXCONN the maximum number of pending connections
        throw(std::runtime_error("Listen failed"));
    }
    // Add server socket to poll monitoring
    struct pollfd server_pfd;
    server_pfd.fd = _server_fd;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    _fds.push_back(server_pfd);
    
    std::cout << "Server initialized and listening on port " << _port << std::endl;
}

void Server::acceptNewClient() {
    int new_fd = accept(_server_fd, NULL, NULL);
    if (new_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN){
            throw(std::runtime_error("Accept failed"));
        }
        return; // No pending connections
    }
    // Make the new socket non-blocking
    if (fcntl(new_fd, F_SETFL, O_NONBLOCK) < 0) {
        close(new_fd);
        throw(std::runtime_error("Set client socket non-blocking failed"));
    }
    // Creating the struct pollfd for the new client
    struct pollfd pfd; 
    pfd.fd = new_fd; // Client socket file descriptor
    pfd.events = POLLIN; // Watch for Data available to read
    pfd.revents = 0; // Clear revents, waiting for POLLIN
    _fds.push_back(pfd); // Add to the list of monitored fds
    // Initialize client data structure
    ClientBuffer client;
    client.fd = new_fd;
    client.buffer = "";
    _clients[new_fd] = client;

    std::cout << "New client connected, fd: " << new_fd << std::endl;
}

void Server::handleClientData(size_t index) {
    int client_fd = _fds[index].fd;
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            throw(std::runtime_error("Receive failed"));
        }
        return;
    } else if (bytes_read == 0) {
        close(client_fd);
        _fds.erase(_fds.begin() + index);
        _clients.erase(client_fd);
        std::cout << "Client disconnected, fd: " << client_fd << std::endl;
        return;
    }
    buffer[bytes_read] = '\0';
    _clients[client_fd].buffer += std::string(buffer, bytes_read);
    
    // Extract and process complete messages
    std::string& clientBuf = _clients[client_fd].buffer;
    size_t pos;
    while ((pos = clientBuf.find("\r\n")) != std::string::npos) {
        std::string message = clientBuf.substr(0, pos);
        clientBuf.erase(0, pos + 2);
        std::cout << "Complete message from fd " << client_fd << ": " << message << std::endl;
        processCommand(client_fd, message);
    }
}

void Server::run(){
    initserver();
    while (true) {
        // poll(the fds vector, number of fds, timeout in ms)
        int poll_count = poll(_fds.data(), _fds.size(), -1);
        if (poll_count < 0) {
            throw(std::runtime_error("Poll failed"));
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) { // Check if there's data to read
                if (_fds[i].fd == _server_fd) {
                    acceptNewClient();
                } else {
                    handleClientData(i);
                }
            }
        }
    }
}

void Server::processCommand(int fd, const std::string& message) {
    // Placeholder
    (void)fd;
    (void)message;
    std::cout << "Processing command: " << message << std::endl;
}