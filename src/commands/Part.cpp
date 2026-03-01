#include "Commands.hpp"
#include "../core/Server.hpp"

void handlePart(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.empty()) {
        server.sendReply(fd, "461", nick, "PART :Not enough parameters");
        return;
    }

    const std::string& channelName = cmd.arguments[0];

    // Optional part message
    std::string reason;
    if (cmd.arguments.size() >= 2) {
        reason = cmd.arguments[1];
    }

    // ERR_NOSUCHCHANNEL (403)
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        server.sendReply(fd, "403", nick, channelName + " :No such channel");
        return;
    }

    // ERR_NOTONCHANNEL (442)
    if (channel->members.find(fd) == channel->members.end()) {
        server.sendReply(fd, "442", nick, channelName + " :You're not on that channel");
        return;
    }

    // Broadcast PART to all channel members (including the parting user)
    std::string partMsg = ":" + nick + "!" + client->username + "@localhost PART " + channelName;
    if (!reason.empty()) {
        partMsg += " :" + reason;
    }
    server.sendToChannel(channelName, partMsg, -1);

    // Remove user from channel
    channel->members.erase(fd);
    channel->operators.erase(fd);
    channel->invited.erase(fd);

    // Remove channel from user's channel list
    client->channels.erase(channelName);

    // If channel is now empty, clean it up
    if (channel->members.empty()) {
        server.removeChannel(channelName);
    }
}
