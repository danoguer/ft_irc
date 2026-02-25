#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>         // std::string
#include <map>            // std::map
#include <set>            // std::set
#include <vector>         // std::vector
#include <stdexcept>      // std::runtime_error
#include <iostream>       // std::cout
#include <ctime>          // time_t, time(), ctime()
#include <csignal>        // signals.

struct IrcCommand;
class Server;

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

// Handler function signature: void handler(Server&, int fd, const IrcCommand&)
typedef void (*CommandHandler)(Server&, int, const IrcCommand&);

struct CommandEntry {
    CommandHandler handler;
    bool requiresRegistration;
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
    void sendReply(int fd, const std::string& code, const std::string& nick, const std::string& rest);
    void sendToChannel(const std::string& channelName, const std::string& message, int excludeFd);
    void disconnectClient(int fd);
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

	// Command dispatch table management
	void addCommand(const std::string& name, const CommandEntry& entry);
	const CommandEntry* findCommand(const std::string& name) const;

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

    // Command dispatch table: maps command name → handler + registration requirement
    std::map<std::string, CommandEntry> _commandMap;
};

#endif