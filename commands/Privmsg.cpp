#include "Privmsg.hpp"

#include "../Server.hpp"

#include <sstream>

static std::string senderPrefix(const Server& server, int senderFd) {
    const std::string nick = server.getNickname(senderFd);
    if (!nick.empty()) {
        return nick;
    }
    std::ostringstream oss;
    oss << "fd" << senderFd;
    return oss.str();
}

void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd) {
    // PRIVMSG <target> <text>

    // handle error when not enough arguments
    if (cmd.arguments.size() < 2) {
        server.sendToClient(senderFd, ":ircserv 461 PRIVMSG :Not enough parameters");
        return;
    }

    const std::string& target = cmd.arguments[0];
    const std::string& text = cmd.arguments[1];

    std::ostringstream line;
    line << ":" << senderPrefix(server, senderFd) << " PRIVMSG " << target << " :" << text;

    if (!target.empty() && target[0] == '#') {
        // broadcast to channel (except sender)
        server.sendToChannel(target, line.str(), senderFd);
        return;
    }

    // direct message
    const int targetFd = server.findClientFdByNickname(target);
    if (targetFd < 0) {
        server.sendToClient(senderFd, ":ircserv 401 " + target + " :No such nick/channel");
        return;
    }
    server.sendToClient(targetFd, line.str());
}
