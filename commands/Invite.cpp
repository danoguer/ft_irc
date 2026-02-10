#include "Invite.hpp"
#include "../Server.hpp"

void handleInvite(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;
    const std::string& srv = server.getServerName();

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.size() < 2) {
        server.sendToClient(fd, ":" + srv + " 461 " + nick + " INVITE :Not enough parameters");
        return;
    }

    const std::string& targetNick = cmd.arguments[0];
    const std::string& channelName = cmd.arguments[1];

    // Look up the channel
    Channel* channel = server.getChannel(channelName);

    // If the channel exists, only members may invite
    if (channel) {
        // ERR_NOTONCHANNEL (442), inviter not in channel
        if (channel->members.find(fd) == channel->members.end()) {
            server.sendToClient(fd, ":" + srv + " 442 " + nick + " " + channelName + " :You're not on that channel");
            return;
        }

        // If channel is invite-only (+i), only operators may invite
        // ERR_CHANOPRIVSNEEDED (482), inviter is not operator
        if (channel->inviteOnly && channel->operators.find(fd) == channel->operators.end()) {
            server.sendToClient(fd, ":" + srv + " 482 " + nick + " " + channelName + " :You're not channel operator");
            return;
        }
    }

    // ERR_NOSUCHNICK (401)
    int targetFd = server.findClientFdByNickname(targetNick);
    if (targetFd < 0) {
        server.sendToClient(fd, ":" + srv + " 401 " + nick + " " + targetNick + " :No such nick/channel");
        return;
    }

    // ERR_USERONCHANNEL (443), target is already on the channel
    if (channel && channel->members.find(targetFd) != channel->members.end()) {
        server.sendToClient(fd, ":" + srv + " 443 " + nick + " " + targetNick + " " + channelName + " :is already on channel");
        return;
    }

    // Add target to the invited set (so they can bypass +i on JOIN)
    // Invite is one time only, if you join and leave you need to be invited again
    if (channel) {
        channel->invited.insert(targetFd);
    }

    // RPL_INVITING (341), send confirmation to the inviter
    server.sendToClient(fd, ":" + srv + " 341 " + nick + " " + targetNick + " " + channelName);

    // Notify the invited user
    server.sendToClient(targetFd, ":" + nick + "!" + client->username + "@localhost INVITE " + targetNick + " " + channelName);
}
