#ifndef BOT_HPP
#define BOT_HPP

#include <string>         // std::string
#include <iostream>       // std::cout
#include <stdexcept>      // std::runtime_error
#include <sys/socket.h>   // socket(), connect(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons()
#include <arpa/inet.h>    // inet_addr()
#include <fcntl.h>        // fcntl(), F_SETFL, O_NONBLOCK
#include <unistd.h>       // close(), usleep()
#include <cerrno>         // errno, EINPROGRESS, EWOULDBLOCK, EAGAIN
#include <ctime>          // time(), ctime()
#include <cstdio>         // popen(), pclose(), fgets()

class Bot {
public:
    Bot(const std::string& server, int port, const std::string& password, const std::string& nickname);
    Bot(const Bot& other);
    Bot& operator=(const Bot& other);
    ~Bot();
    void connectToServer(const std::string& server, int port);
    void authenticate();
    void run();
    void receiveMessage();
    void sendMessage(const std::string& message);
    void handleMessage(const std::string& message);
    std::string getWeather(const std::string& city);
    
private:
    int _botfd;
    std::string _server;
    int _port;
    std::string _password;
    std::string _nickname;
    std::string _buffer;
};

#endif