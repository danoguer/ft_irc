#include "Commands.hpp"
#include "../core/Server.hpp"

void handleTopic(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.empty()) {
        server.sendReply(fd, "461", nick, "TOPIC :Not enough parameters");
        return;
    }

    const std::string& channelName = cmd.arguments[0];

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

    // If there's a single argument, we want to recover the topic instead of setting it
    if (cmd.arguments.size() < 2) {
        if (channel->topic.empty()) {
            // RPL_NOTOPIC (331)
            server.sendReply(fd, "331", nick, channelName + " :No topic is set");
        } else {
            // RPL_TOPIC (332)
            server.sendReply(fd, "332", nick, channelName + " :" + channel->topic);
        }
        return;
    }

    // Setting topic

    // ERR_CHANOPRIVSNEEDED (482), if +t, only operators can change topic
    if (channel->topicRestricted && channel->operators.find(fd) == channel->operators.end()) {
        server.sendReply(fd, "482", nick, channelName + " :You're not channel operator");
        return;
    }

    // Set the topic (to clear it, send an empty string)
    channel->topic = cmd.arguments[1];

    // Broadcast the topic change to all channel members
    std::string topicMsg = ":" + nick + "!" + client->username + "@localhost TOPIC " + channelName + " :" + channel->topic;
    server.sendToChannel(channelName, topicMsg, -1);
}
