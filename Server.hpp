#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>         // std::string
#include <map>            // std::map
#include <set>            // std::set
#include <stdexcept>      // std::runtime_error
#include <iostream>       // std::cout

struct IrcCommand;

#include "Network.hpp"

struct Client {
    int fd;
    std::string nickname;
    bool authenticated;
    std::set<std::string> channels;
};

struct Channel {
    std::string name;
    std::set<int> members;
    std::string topic;
};

class Server {
public:
    Server(int port, const std::string& password);
    Server(const Server& other);
    Server& operator=(const Server& other);
    ~Server();
    
    void run();
    void onClientConnect(int fd);
    void onClientDisconnect(int fd);
    void processCommand(int fd, const std::string& message);
    void sendToClient(int fd, const std::string& message);
    void sendToChannel(const std::string& channelName, const std::string& message, int excludeFd);

	int findClientFdByNickname(const std::string& nickname) const;
	std::string getNickname(int fd) const;
private:
	void executeCommand(int fd, const IrcCommand& cmd);

    Network _network;
    std::map<int, Client> _clients;
    std::map<std::string, Channel> _channels;
    std::string _password;
};

#endif