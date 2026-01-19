#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

struct ClientBuffer {
    int fd;
    std::string buffer;
};

class Server {
public:
    Server(int port);
    ~Server();
    void run();
    void initserver();
    void acceptNewClient();
    void handleClientData(size_t index);
    void processCommand(int fd, const std::string& message);
private:
    int _server_fd;
    int _port;
    std::vector<struct pollfd> _fds;
    std::map<int, ClientBuffer> _clients;
};

#endif