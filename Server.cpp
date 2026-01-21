#include "Server.hpp"

Server::Server(int port, const std::string& password) : _network(port), _password(password) {
    if (password.empty()) {
        throw(std::runtime_error("Password cannot be empty"));
    }
    if (password.length() < 8) {
        throw(std::runtime_error("Password must be at least 8 characters long"));
    }
}

Server::Server(const Server& other) : _network(6667), _password(other._password) {
    (void)other;
}

Server& Server::operator=(const Server& other) {
    (void)other;
    return *this;
}

Server::~Server() {}

void Server::run(){
    _network.initNetwork();
    while (true) {
        // Poll network events and handle them
        std::vector<NetworkEvent> events = _network.getEvents();
        // Handle each event
        for (size_t i = 0; i < events.size(); ++i) {
            switch (events[i].type) {
                case EVENT_NEW_CONNECTION:
                    onClientConnect(events[i].client_fd);
                    break;
                case EVENT_CLIENT_DATA:
                    processCommand(events[i].client_fd, events[i].message);
                    break;
                case EVENT_CLIENT_DISCONNECT:
                    onClientDisconnect(events[i].client_fd);
                    _network.disconnectClient(events[i].client_fd);
                    break;
                case EVENT_NONE:
                    break;
            }
        }
    }
}

void Server::onClientConnect(int fd) {
    // Initialize client data structure
    Client client;
    client.fd = fd;
    client.nickname = "";
    client.authenticated = false;
    _clients[fd] = client;
}

void Server::onClientDisconnect(int fd) {
    // Clean up client data structure
    if (_clients.find(fd) != _clients.end()) {
        // Remove client from all channels
        std::set<std::string>& client_channels = _clients[fd].channels;
        for (std::set<std::string>::iterator it = client_channels.begin(); it != client_channels.end(); ++it) {
            if (_channels.find(*it) != _channels.end()) {
                _channels[*it].members.erase(fd);
            }
        }
        // Remove client from clients map
        _clients.erase(fd);
    }
}

void Server::sendToChannel(const std::string& channelName, const std::string& message, int excludeFd) {
    if (_channels.find(channelName) == _channels.end()) {
        return;
    }
    Channel& channel = _channels[channelName];
    for (std::set<int>::iterator it = channel.members.begin(); it != channel.members.end(); ++it) {
        if (*it != excludeFd) {
            _network.sendToClient(*it, message);
        }
    }
}

void Server::processCommand(int fd, const std::string& message) {
    // Placeholder
    (void)fd;
    (void)message;
    std::cout << "Processing command: " << message << std::endl;
}