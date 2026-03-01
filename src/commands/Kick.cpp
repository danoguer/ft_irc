#include "Commands.hpp"
#include "../core/Server.hpp"

void handleKick(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.size() < 2) {
        server.sendReply(fd, "461", nick, "KICK :Not enough parameters");
        return;
    }

    const std::string& channelName = cmd.arguments[0];
    const std::string& targetNick = cmd.arguments[1];

    // Kick reason (defaults to kicker's nickname per RFC)
    std::string reason = nick;
    if (cmd.arguments.size() >= 3) {
        reason = cmd.arguments[2];
    }

    // ERR_NOSUCHCHANNEL (403)
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        server.sendReply(fd, "403", nick, channelName + " :No such channel");
        return;
    }

    // ERR_NOTONCHANNEL (442), kicker must be on the channel
    if (channel->members.find(fd) == channel->members.end()) {
        server.sendReply(fd, "442", nick, channelName + " :You're not on that channel");
        return;
    }

    // ERR_CHANOPRIVSNEEDED (482), only operators can kick
    if (channel->operators.find(fd) == channel->operators.end()) {
        server.sendReply(fd, "482", nick, channelName + " :You're not channel operator");
        return;
    }

    // ERR_USERNOTINCHANNEL (441), target must be on the channel
    int targetFd = server.findClientFdByNickname(targetNick);
    if (targetFd < 0 || channel->members.find(targetFd) == channel->members.end()) {
        server.sendReply(fd, "441", nick, targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    // Broadcast KICK to all channel members (including kicker and target)
    std::string kickMsg = ":" + nick + "!" + client->username + "@localhost KICK " + channelName + " " + targetNick + " :" + reason;
    server.sendToChannel(channelName, kickMsg, -1);

    // Remove target from channel
    channel->members.erase(targetFd);
    channel->operators.erase(targetFd);
    channel->invited.erase(targetFd);

    // Remove channel from target's channel list
    Client* targetClient = server.getClient(targetFd);
    if (targetClient) {
        targetClient->channels.erase(channelName);
    }
}
