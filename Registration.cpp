#include "Server.hpp"

bool Server::checkPassword(const std::string& password) const {
    return password == _password;
}

// to be registered, a user needs to:
// - provide the server password (with PASS)
// - provide a nickname (with NICK)
// - provide a username and real name (with USER)
// all IRC commands (except PASS, NICK and USER) will be blocked until registration is complete
// according to the standard, 
// - registration requires both NICK and USER 
// - NICK and USER can come in any order
// - registration can't complete if PASS isn't accepted
// finally, username saving and persistence across sessions is not part of IRC protocol, and is handled by an external bot
void Server::tryCompleteRegistration(int fd) {
    Client* client = getClient(fd);
    if (!client || client->registered) {
        return;
    }

    if (!_password.empty() && !client->passAccepted) {
        return;
    }
    if (client->nickname.empty()) {
        return;
    }
    if (client->username.empty()) {
        return;
    }

    client->registered = true;
    sendWelcome(fd);
}

// once registration is complete, server sends 001-004 welcome block followed by MOTD
void Server::sendWelcome(int fd) {
    Client* client = getClient(fd);
    if (!client) {
        return;
    }

    const std::string& nick = client->nickname;
    const std::string& user = client->username;

    // format server creation time: strip trailing newline from ctime
    std::string createdStr = std::ctime(&_createdAt);
    if (!createdStr.empty() && createdStr[createdStr.size() - 1] == '\n') {
        createdStr.erase(createdStr.size() - 1);
    }

    // RPL_WELCOME (001)
    sendToClient(fd, ":" + _serverName + " 001 " + nick + " :Welcome to the ft_irc network " + nick + "!" + user + "@localhost");
    // RPL_YOURHOST (002)
    sendToClient(fd, ":" + _serverName + " 002 " + nick + " :Your host is " + _serverName + ", running version 1.0");
    // RPL_CREATED (003)
    sendToClient(fd, ":" + _serverName + " 003 " + nick + " :This server was created " + createdStr);
    // RPL_MYINFO (004): <servername> <version> <user modes> <channel modes>
    // user modes: we don't implement any
    // channel modes: i t k o l
    sendToClient(fd, ":" + _serverName + " 004 " + nick + " " + _serverName + " 1.0 o itkol");
    // MOTD (375/372/376 or 422 if missing)
    sendMotd(fd);
}
