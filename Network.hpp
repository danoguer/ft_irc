#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

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
