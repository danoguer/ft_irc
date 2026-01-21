#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>         // std::string
#include <vector>         // std::vector
#include <map>            // std::map
#include <stdexcept>      // std::runtime_error
#include <iostream>       // std::cout, std::cerr
#include <poll.h>         // poll(), pollfd, POLLIN
#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv(), setsockopt()
#include <netinet/in.h>   // sockaddr_in, INADDR_ANY, htons(), SOMAXCONN
#include <fcntl.h>        // fcntl(), F_SETFL, O_NONBLOCK
#include <unistd.h>       // close()
#include <cerrno>         // errno, EWOULDBLOCK, EAGAIN
#include <cstring>        // memset()

enum EventType {
    EVENT_NONE,
    EVENT_NEW_CONNECTION,
    EVENT_CLIENT_DATA,
    EVENT_CLIENT_DISCONNECT
};

struct NetworkEvent {
    EventType type;
    int client_fd;
    std::string message;
};

class Network {
public:
    Network(int port);
    Network(const Network& other);
    Network& operator=(const Network& other);
    ~Network();
    
    void initNetwork();
    std::vector<NetworkEvent> getEvents();
    void disconnectClient(int fd);
    void sendToClient(int fd, const std::string& message);

private:
    int acceptNewClient();
    int receiveFromClient(int fd, std::string& message);

    int _server_fd;
    int _port;
    std::vector<struct pollfd> _fds;
    std::map<int, std::string> _client_buffer;
};

#endif
