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

// once registration is complete, server sends a multipart registration message
// TODO complete this message with MOTD (see left sidebar in https://dd.ircdocs.horse/refs/numerics/372)
// TODO complete 004 message with the list of channel modes we implement (https://dd.ircdocs.horse/refs/numerics/004)
// TODO replace placeholder in 003 message with actual server startup time
void Server::sendWelcome(int fd) {
    Client* client = getClient(fd);
    if (!client) {
        return;
    }

    const std::string& nick = client->nickname;
    const std::string& user = client->username;

    // RPL_WELCOME (001)
    sendToClient(fd, ":server 001 " + nick + " :Welcome to the ft_irc network " + nick + "!" + user + "@localhost");
    // RPL_YOURHOST (002)
    sendToClient(fd, ":server 002 " + nick + " :Your host is ircserv, running version 1.0");
    // RPL_CREATED (003)
    sendToClient(fd, ":server 003 " + nick + " :This server was created today");
    // RPL_MYINFO (004)
    sendToClient(fd, ":server 004 " + nick + " ircserv v1.0 o o");
}
