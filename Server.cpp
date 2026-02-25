#include "Server.hpp"
#include "commands/Commands.hpp"
#include "IrcParser.hpp"

#include <algorithm>
#include <fstream>

static volatile bool server_shutdown = false;

Server::Server(int port, const std::string& password)
    : _network(port), _password(password), _serverName("ircserv"), _createdAt(std::time(NULL)) {
    // empty password is allowed per standard, but we don't want weak passwords
    if (!password.empty() && password.length() < 8) {
        throw(std::runtime_error("Password must be at least 8 characters long"));
    }
    // Build the command dispatch table
    initCommandMap(*this);
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

static void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT)
        server_shutdown = true;
}

void Server::run(){
    _network.initNetwork();
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill command default
    signal(SIGQUIT, signalHandler);  // Ctrl+backslash
    signal(SIGPIPE, SIG_IGN);        // Ignore broken pipe (handle via send() errors)
    while (!server_shutdown) {
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
    // SIGINT received - clean shutdown
    std::cout << "\nShutting down server..." << std::endl;
    // Send quit message to all connected clients and close their FDs
    std::vector<int> client_fds;
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        client_fds.push_back(it->first);
    }
    for (size_t i = 0; i < client_fds.size(); ++i) {
        sendToClient(client_fds[i], "ERROR :Server shutting down");
        _network.disconnectClient(client_fds[i]);
    }
    _clients.clear();
    std::cout << "All client connections closed" << std::endl;
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

void Server::sendReply(int fd, const std::string& code, const std::string& nick, const std::string& rest) {
    _network.sendToClient(fd, ":" + _serverName + " " + code + " " + nick + " " + rest);
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
        sendReply(fd, "422", nick, ":MOTD File is missing");
        return;
    }

    // RPL_MOTDSTART (375)
    sendReply(fd, "375", nick, ":- " + _serverName + " Message of the day - ");
    // RPL_MOTD (372), one per line
    for (size_t i = 0; i < _motdLines.size(); ++i) {
        sendReply(fd, "372", nick, ":- " + _motdLines[i]);
    }
    // RPL_ENDOFMOTD (376)
    sendReply(fd, "376", nick, ":End of MOTD command");
}

void Server::addCommand(const std::string& name, const CommandEntry& entry) {
    _commandMap[name] = entry;
}

const CommandEntry* Server::findCommand(const std::string& name) const {
    std::map<std::string, CommandEntry>::const_iterator it = _commandMap.find(name);
    if (it == _commandMap.end()) {
        return NULL;
    }
    return &(it->second);
}

void Server::executeCommand(int fd, const IrcCommand& cmd) {
    // Look up command in dispatch table
    const CommandEntry* entry = findCommand(cmd.command);
    if (!entry) {
        // ERR_UNKNOWNCOMMAND (421)
        Client* client = getClient(fd);
        std::string who = (client && !client->nickname.empty()) ? client->nickname : "*";
        sendReply(fd, "421", who, cmd.command + " :Unknown command");
        return;
    }

    // Check registration requirement
    if (entry->requiresRegistration) {
        Client* client = getClient(fd);
        if (!client || !client->registered) {
            std::string who = (client && !client->nickname.empty()) ? client->nickname : "*";
            sendReply(fd, "451", who, ":You have not registered");
            return;
        }
    }

    // Dispatch to handler
    entry->handler(*this, fd, cmd);
}

// IRC commands are case-insensitive (RFC 2812 §2.3)
static std::string toUpper(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::toupper);
    return out;
}

void Server::processCommand(int fd, const std::string& message) {
    IrcCommand cmd;
    // parse command
    if (!parseIrcCommandLine(message, cmd)) {
        std::cerr << "Invalid IRC message from fd " << fd << ": '" << message << "'" << std::endl;
        return;
    }

    // normalize command to uppercase (IRC commands are case-insensitive)
    cmd.command = toUpper(cmd.command);

    // parser debug
    std::cout << "[" << fd
              << "] " << cmd.command;
    for (size_t idx = 0; idx < cmd.arguments.size(); ++idx) {
        std::cout << " " << cmd.arguments[idx];
    }
    std::cout << std::endl;

    // execute command
    executeCommand(fd, cmd);
}