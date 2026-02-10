#include "Nick.hpp"
#include "../Server.hpp"

// check if nickname is valid per RFC 2812
// nickname = ( letter / special ) *8( letter / digit / special / "-" )
// special = "[", "]", "\", "`", "_", "^", "{", "|", "}"
static bool isLetterOrDigit(char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
}

static bool isNickSpecialChar(char c) {
    switch (c) {
        case '[': case ']': case '\\': case '`':
        case '_': case '^': case '{': case '|': case '}':
            return true;
        default:
            return false;
    }
}

static bool isNickFirstChar(char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) || isNickSpecialChar(c);
}

static bool isNickChar(char c) {
    return isLetterOrDigit(c) || isNickSpecialChar(c) || c == '-';
}

static bool isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > 9) {
        return false;
    }

    if (!isNickFirstChar(nick[0])) {
        return false;
    }

    for (size_t i = 1; i < nick.length(); ++i) {
        if (!isNickChar(nick[i])) {
            return false;
        }
    }
    return true;
}

void handleNick(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return;
    }
    std::string who = client->nickname.empty() ? "*" : client->nickname;

    // ERR_NONICKNAMEGIVEN (431)
    if (cmd.arguments.empty()) {
        server.sendToClient(fd, ":" + server.getServerName() + " 431 " + who + " :No nickname given");
        return;
    }

    const std::string& newNick = cmd.arguments[0];

    // ERR_ERRONEUSNICKNAME (432)
    if (!isValidNickname(newNick)) {
        server.sendToClient(fd, ":" + server.getServerName() + " 432 " + who + " " + newNick + " :Erroneous nickname");
        return;
    }

    // ERR_NICKNAMEINUSE (433)
    int existingFd = server.findClientFdByNickname(newNick);
    if (existingFd != -1 && existingFd != fd) {
        server.sendToClient(fd, ":" + server.getServerName() + " 433 " + who + " " + newNick + " :Nickname is already in use");
        return;
    }

    std::string oldNick = client->nickname;
    client->nickname = newNick;

    // if already registered, notify user and all channels the user is in 
    // about their nick change
    if (client->registered && !oldNick.empty()) {
        // send NICK change notification to user
        std::string notification = ":" + oldNick + " NICK " + newNick;
        server.sendToClient(fd, notification);
        // notify all channels the user is in
        for (std::set<std::string>::iterator it = client->channels.begin(); it != client->channels.end(); ++it) {
            server.sendToChannel(*it, notification, fd); // send to all in channel except the user
        }
    }

    // try to complete registration if not yet registered
    if (!client->registered) {
        server.tryCompleteRegistration(fd);
    }
}
