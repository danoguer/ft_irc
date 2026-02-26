#include "Commands.hpp"

#include "../Server.hpp"

#include <sstream>

// Shared logic for PRIVMSG and NOTICE.
// When isNotice is true, error replies are suppressed (RFC 2812 §3.3.2).
static void handleMessage(Server& server, int senderFd, const IrcCommand& cmd,
                           const std::string& verb, bool isNotice) {
    Client* client = server.getClient(senderFd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    if (cmd.arguments.size() < 2) {
        if (!isNotice)
            server.sendReply(senderFd, "461", nick, verb + " :Not enough parameters");
        return;
    }

    const std::string& target = cmd.arguments[0];
    const std::string& text = cmd.arguments[1];

    std::ostringstream line;
    line << ":" << senderPrefix(server, senderFd) << " " << verb << " " << target << " :" << text;

    if (!target.empty() && target[0] == '#') {
        Channel* channel = server.getChannel(target);
        if (!channel) {
            if (!isNotice)
                server.sendReply(senderFd, "403", nick, target + " :No such channel");
            return;
        }
        if (channel->members.find(senderFd) == channel->members.end()) {
            if (!isNotice)
                server.sendReply(senderFd, "404", nick, target + " :Cannot send to channel");
            return;
        }
        server.sendToChannel(target, line.str(), senderFd);
        return;
    }

    const int targetFd = server.findClientFdByNickname(target);
    if (targetFd < 0) {
        if (!isNotice)
            server.sendReply(senderFd, "401", nick, target + " :No such nick/channel");
        return;
    }
    server.sendToClient(targetFd, line.str());
}

void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd) {
    handleMessage(server, senderFd, cmd, "PRIVMSG", false);
}

void handleNotice(Server& server, int senderFd, const IrcCommand& cmd) {
    handleMessage(server, senderFd, cmd, "NOTICE", true);
}
