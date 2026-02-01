#include "Server.hpp"

#include "IrcParser.hpp"

#include "commands/Privmsg.hpp"
#include "commands/Pass.hpp"
#include "commands/Nick.hpp"
#include "commands/User.hpp"

Server::Server(int port, const std::string& password) : _network(port), _password(password) {
    // empty password is allowed per standard, but we don't want weak passwords
    if (!password.empty() && password.length() < 8) {
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

Client* Server::getClient(int fd) {
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) {
        return NULL;
    }
    return &(it->second);
}

void Server::executeCommand(int fd, const IrcCommand& cmd) {
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
        sendToClient(fd, ":server 451 " + who + " :You have not registered");
        return;
    }

    if (cmd.command == "PRIVMSG") {
        handlePrivmsg(*this, fd, cmd);
        return;
    }
    
    // handle unknown commands
    // NOTE: to be completely standards compliant, this needs to go BEFORE the
    // registration check (i.e. if we get an unknown command while unregistered
    // we need to reply 421 not 451)
    // but to do this we have to know the full list of commands we handle
    // so, TODO fix this last
    if (!cmd.command.empty()) {
        Client* client = getClient(fd);
        std::string who = (client && !client->nickname.empty()) ? client->nickname : "*";
        sendToClient(fd, ":server 421 " + who + " " + cmd.command + " :Unknown command");
        return;
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