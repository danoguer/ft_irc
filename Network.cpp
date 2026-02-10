#include "Network.hpp"

Network::Network(int port) : _server_fd(-1), _port(port) {
    if (port <= 0 || port > 65535) {
        throw(std::runtime_error("Invalid port number"));
    }
}

Network::Network(const Network& other) : _server_fd(-1), _port(other._port) {
    (void)other;
}

Network& Network::operator=(const Network& other) {
    (void)other;
    return *this;
}

Network::~Network() {
    // Close all client connections
    for (std::map<int, std::string>::iterator it = _client_buffer.begin(); it != _client_buffer.end(); ++it) {
        close(it->first);
    }
    // Close server socket
    if (_server_fd != -1) {
        close(_server_fd);
    }
}

void Network::initNetwork(){
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
    std::cout << "Network initialized and listening on port " << _port << std::endl;
}

int Network::acceptNewClient() {
    int new_fd = accept(_server_fd, NULL, NULL);
    if (new_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN){
            throw(std::runtime_error("Accept failed"));
        }
        return -1; // No pending connections
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
    // Initialize buffer for this client
    _client_buffer[new_fd] = ""; 
    std::cout << "New client connected, fd: " << new_fd << std::endl;
    return new_fd;
}

int Network::receiveFromClient(int fd, std::vector<std::string>& messages) {
    char buffer[1024];   
    // recv reads data from the socket
    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        // EWOULDBLOCK/EAGAIN/EINTR are not fatal in non-blocking mode
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
            return 0;
        }
        // Treat common connection errors as disconnects
        if (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE) {
            return -1;
        }
        throw(std::runtime_error("Receive failed"));
    } else if (bytes_read == 0) {
        return -1; // Client disconnected
    }
    buffer[bytes_read] = '\0';
    _client_buffer[fd] += std::string(buffer, bytes_read);
    // Extract multiple messages (handle \r\n or bare \n)
    std::string& clientBuf = _client_buffer[fd];
    size_t pos = 0;
    bool found_message = false;
    // While we can find a '\n', keep extracting messages
    while ((pos = clientBuf.find('\n')) != std::string::npos) {
        std::string msg = clientBuf.substr(0, pos);
        if (!msg.empty() && msg[msg.size() - 1] == '\r') {
            msg.erase(msg.size() - 1);
        }
        messages.push_back(msg);       // Add to the vector
        clientBuf.erase(0, pos + 1);   // Remove up to and including '\n'
        found_message = true;
    }
    return found_message ? 1 : 0;
}

void Network::disconnectClient(int fd) {
    // Find and remove from poll fds
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    // Remove buffer and close socket
    _client_buffer.erase(fd);
    close(fd);
}

void Network::sendToClient(int fd, const std::string& message) {
    std::string toSend = message;
    // Ensure message ends with \r\n adding if not present
    if (toSend.length() < 2 || toSend.substr(toSend.length() - 2) != "\r\n") {
        toSend += "\r\n";
    }
    //send(fd, message.c_str(), message.length(), 0)
    ssize_t bytes_sent = send(fd, toSend.c_str(), toSend.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send to client fd " << fd << std::endl;
    }
}

std::vector<NetworkEvent> Network::getEvents() {
    std::vector<NetworkEvent> events;
    // poll(the fds vector, number of fds, timeout in ms)
    int poll_count = poll(_fds.data(), _fds.size(), -1);
    if (poll_count < 0) {
        throw(std::runtime_error("Poll failed"));
    }
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            if (_fds[i].fd != _server_fd) {
                NetworkEvent event;
                event.type = EVENT_CLIENT_DISCONNECT;
                event.client_fd = _fds[i].fd;
                events.push_back(event);
            }
            continue;
        }
        if (_fds[i].revents & POLLIN) {
            if (_fds[i].fd == _server_fd) {
                // New Connection Handling
                int new_fd = acceptNewClient();
                if (new_fd > 0) {
                    NetworkEvent event;
                    event.type = EVENT_NEW_CONNECTION;
                    event.client_fd = new_fd;
                    events.push_back(event);
                }
            } else {
                // Client Data Handling
                int client_fd = _fds[i].fd;       
                // Prepare a vector to hold multiple potential messages
                std::vector<std::string> messages;
                // Pass the vector to receiveFromClient
                int status = receiveFromClient(client_fd, messages);
                if (status == -1) {
                    // Client disconnected
                    NetworkEvent event;
                    event.type = EVENT_CLIENT_DISCONNECT;
                    event.client_fd = client_fd;
                    events.push_back(event);
                } 
                else if (status == 1) {
                    // Message(s) received
                    // We loop through ALL messages extracted from the buffer
                    for (size_t j = 0; j < messages.size(); j++) {
                        NetworkEvent event;
                        event.type = EVENT_CLIENT_DATA;
                        event.client_fd = client_fd;
                        event.message = messages[j];
                        events.push_back(event);
                    }
                }
            }
        }
    }
    return events;
}
