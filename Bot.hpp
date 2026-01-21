#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctime>
#include <cstdio>

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