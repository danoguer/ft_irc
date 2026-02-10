#include "Commands.hpp"

#include "../Server.hpp"

#include <sstream>

static std::string senderPrefix(Server& server, int senderFd) {
    Client* client = server.getClient(senderFd);
    if (!client || client->nickname.empty()) {
        std::ostringstream oss;
        oss << "fd" << senderFd;
        return oss.str();
    }
    return client->nickname + "!" + client->username + "@localhost";
}

void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd) {
    // PRIVMSG <target> <text>
    Client* client = server.getClient(senderFd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    // handle error when not enough arguments
    if (cmd.arguments.size() < 2) {
        server.sendReply(senderFd, "461", nick, "PRIVMSG :Not enough parameters");
        return;
    }

    const std::string& target = cmd.arguments[0];
    const std::string& text = cmd.arguments[1];

    std::ostringstream line;
    line << ":" << senderPrefix(server, senderFd) << " PRIVMSG " << target << " :" << text;

    if (!target.empty() && target[0] == '#') {
        // channel message — sender must be a member
        Channel* channel = server.getChannel(target);
        if (!channel) {
            server.sendReply(senderFd, "403", nick, target + " :No such channel");
            return;
        }
        if (channel->members.find(senderFd) == channel->members.end()) {
            // ERR_CANNOTSENDTOCHAN (404)
            server.sendReply(senderFd, "404", nick, target + " :Cannot send to channel");
            return;
        }
        // broadcast to channel (except sender)
        server.sendToChannel(target, line.str(), senderFd);
        return;
    }

    // direct message
    const int targetFd = server.findClientFdByNickname(target);
    if (targetFd < 0) {
        server.sendReply(senderFd, "401", nick, target + " :No such nick/channel");
        return;
    }
    server.sendToClient(targetFd, line.str());
}
