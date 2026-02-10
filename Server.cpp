#include "Server.hpp"

#include "IrcParser.hpp"
#include <fstream>

#include "commands/Privmsg.hpp"
#include "commands/Pass.hpp"
#include "commands/Nick.hpp"
#include "commands/User.hpp"
#include "commands/Join.hpp"
#include "commands/Part.hpp"
#include "commands/Mode.hpp"
#include "commands/Invite.hpp"
#include "commands/Kick.hpp"
#include "commands/Topic.hpp"

// Pre-registration commands (allowed before NICK+USER handshake completes)
const std::string Server::_preRegCommands[] = { "PASS", "NICK", "USER" };
const size_t Server::_preRegCount = sizeof(Server::_preRegCommands) / sizeof(Server::_preRegCommands[0]);

// Post-registration commands (require completed registration)
const std::string Server::_postRegCommands[] = { "PRIVMSG", "JOIN", "PART", "MODE", "INVITE", "KICK", "TOPIC", "MOTD" };
const size_t Server::_postRegCount = sizeof(Server::_postRegCommands) / sizeof(Server::_postRegCommands[0]);

bool Server::isKnownCommand(const std::string& command) const {
    for (size_t i = 0; i < _preRegCount; ++i) {
        if (_preRegCommands[i] == command)
            return true;
    }
    for (size_t i = 0; i < _postRegCount; ++i) {
        if (_postRegCommands[i] == command)
            return true;
    }
    return false;
}

bool Server::requiresRegistration(const std::string& command) const {
    for (size_t i = 0; i < _postRegCount; ++i) {
        if (_postRegCommands[i] == command)
            return true;
    }
    return false;
}

Server::Server(int port, const std::string& password)
    : _network(port), _password(password), _serverName("ircserv"), _createdAt(std::time(NULL)) {
    // empty password is allowed per standard, but we don't want weak passwords
    if (!password.empty() && password.length() < 8) {
        throw(std::runtime_error("Password must be at least 8 characters long"));
    }
    // try to load MOTD from several paths (relative to cwd, then /etc)
    // missing MOTD is not an error, clients will just get ERR_NOMOTD (422)
    if (!loadMotd("motd.txt")) {
        loadMotd("/etc/ircserv/motd.txt");
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
    client.username = "";
    client.realname = "";
    client.passAccepted = false;
    client.registered = false;
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

void Server::sendToClient(int fd, const std::string& message) {
    _network.sendToClient(fd, message);
}

int Server::findClientFdByNickname(const std::string& nickname) const {
    for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.nickname == nickname) {
            return it->first;
        }
    }
    return -1;
}

std::string Server::getNickname(int fd) const {
    std::map<int, Client>::const_iterator it = _clients.find(fd);
    if (it == _clients.end()) {
        return "";
    }
    return it->second.nickname;
}

const std::string& Server::getServerName() const {
    return _serverName;
}

Client* Server::getClient(int fd) {
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) {
        return NULL;
    }
    return &(it->second);
}

Channel* Server::getChannel(const std::string& name) {
    std::map<std::string, Channel>::iterator it = _channels.find(name);
    if (it == _channels.end()) {
        return NULL;
    }
    return &(it->second);
}

Channel& Server::createChannel(const std::string& name) {
    _channels[name] = Channel();
    _channels[name].name = name;
    return _channels[name];
}

void Server::removeChannel(const std::string& name) {
    _channels.erase(name);
}

bool Server::loadMotd(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    _motdLines.clear();
    std::string line;
    while (std::getline(file, line)) {
        // strip trailing \r for cross-platform compatibility
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        _motdLines.push_back(line);
    }
    return true;
}

void Server::sendMotd(int fd) {
    Client* client = getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    if (_motdLines.empty()) {
        // ERR_NOMOTD (422)
        sendToClient(fd, ":" + _serverName + " 422 " + nick + " :MOTD File is missing");
        return;
    }

    // RPL_MOTDSTART (375)
    sendToClient(fd, ":" + _serverName + " 375 " + nick + " :- " + _serverName + " Message of the day - ");
    // RPL_MOTD (372), one per line
    for (size_t i = 0; i < _motdLines.size(); ++i) {
        sendToClient(fd, ":" + _serverName + " 372 " + nick + " :- " + _motdLines[i]);
    }
    // RPL_ENDOFMOTD (376)
    sendToClient(fd, ":" + _serverName + " 376 " + nick + " :End of MOTD command");
}

void Server::executeCommand(int fd, const IrcCommand& cmd) {
    // Check for unknown commands
    if (!isKnownCommand(cmd.command)) {
        Client* client = getClient(fd);
        std::string who = (client && !client->nickname.empty()) ? client->nickname : "*";
        sendToClient(fd, ":" + _serverName + " 421 " + who + " " + cmd.command + " :Unknown command");
        return;
    }

    // PASS, NICK, USER are allowed before registration
    if (cmd.command == "PASS") {
        handlePass(*this, fd, cmd);
        return;
    }
    if (cmd.command == "NICK") {
        handleNick(*this, fd, cmd);
        return;
    }
    if (cmd.command == "USER") {
        handleUser(*this, fd, cmd);
        return;
    }

    // all other commands require registration
    Client* client = getClient(fd);
    if (!client || !client->registered) {
        std::string who = (client && !client->nickname.empty()) ? client->nickname : "*";
        sendToClient(fd, ":" + _serverName + " 451 " + who + " :You have not registered");
        return;
    }

    if (cmd.command == "PRIVMSG") {
        handlePrivmsg(*this, fd, cmd);
    } else if (cmd.command == "JOIN") {
        handleJoin(*this, fd, cmd);
    } else if (cmd.command == "PART") {
        handlePart(*this, fd, cmd);
    } else if (cmd.command == "MODE") {
        handleMode(*this, fd, cmd);
    } else if (cmd.command == "INVITE") {
        handleInvite(*this, fd, cmd);
    } else if (cmd.command == "KICK") {
        handleKick(*this, fd, cmd);
    } else if (cmd.command == "TOPIC") {
        handleTopic(*this, fd, cmd);
    } else if (cmd.command == "MOTD") {
        sendMotd(fd);
    }
}

void Server::processCommand(int fd, const std::string& message) {
    IrcCommand cmd;
    // parse command
    if (!parseIrcCommandLine(message, cmd)) {
        std::cerr << "Invalid IRC message from fd " << fd << ": '" << message << "'" << std::endl;
        return;
    }

    // parser debug
    std::cout << "Processing IRC command from fd " << fd
              << " | command='" << cmd.command << "'";
    for (size_t idx = 0; idx < cmd.arguments.size(); ++idx) {
        std::cout << " arg[" << idx << "]='" << cmd.arguments[idx] << "'";
    }
    std::cout << std::endl;

    // execute command
    executeCommand(fd, cmd);
}