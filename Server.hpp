#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>         // std::string
#include <map>            // std::map
#include <set>            // std::set
#include <vector>         // std::vector
#include <stdexcept>      // std::runtime_error
#include <iostream>       // std::cout
#include <ctime>          // time_t, time(), ctime()

struct IrcCommand;

#include "Network.hpp"

struct Client {
    int fd;
    std::string nickname;
    std::string username;
    std::string realname;
    bool passAccepted;  // PASS command succeeded
    bool registered;    // completed NICK + USER sequence
    std::set<std::string> channels;
};

struct Channel {
    std::string name;
    std::set<int> members;
    std::set<int> operators;
    std::set<int> invited;     // fds that have been INVITEd
    std::string topic;
    std::string key;           // channel password (mode +k)
    int userLimit;             // 0 = no limit (mode +l)
    bool inviteOnly;           // mode +i
    bool topicRestricted;      // mode +t: only ops can set topic

    Channel() : userLimit(0), inviteOnly(false), topicRestricted(false) {}
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
	const std::string& getServerName() const;
	Client* getClient(int fd);
	Channel* getChannel(const std::string& name);
	Channel& createChannel(const std::string& name);
	void removeChannel(const std::string& name);
	bool checkPassword(const std::string& password) const;
	void tryCompleteRegistration(int fd);
	void sendMotd(int fd);
	bool isKnownCommand(const std::string& command) const;
	bool requiresRegistration(const std::string& command) const;

private:
	void executeCommand(int fd, const IrcCommand& cmd);
	void sendWelcome(int fd);
	bool loadMotd(const std::string& path);

    Network _network;
    std::map<int, Client> _clients;
    std::map<std::string, Channel> _channels;
    std::string _password;
    std::string _serverName;
    time_t _createdAt;
    std::vector<std::string> _motdLines;

    // Commands allowed before registration (during handshake)
    // All other known commands require registration first
    static const std::string _preRegCommands[];     // PASS, NICK, USER
    static const std::string _postRegCommands[];    // PRIVMSG, JOIN, PART, MODE, INVITE, KICK, TOPIC, MOTD
    static const size_t _preRegCount;
    static const size_t _postRegCount;
};

#endif