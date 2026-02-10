#include "Commands.hpp"
#include "../Server.hpp"

#include <sstream>

// Validate channel name: must start with '#', 2-50 chars, no spaces/commas/ctrl
static bool isValidChannelName(const std::string& name) {
    if (name.size() < 2 || name.size() > 50) {
        return false;
    }
    if (name[0] != '#') {
        return false;
    }
    for (size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (c == ' ' || c == ',' || c == '\x07' || c == '\r' || c == '\n') {
            return false;
        }
    }
    return true;
}

static std::string senderPrefix(Server& server, int fd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return "*";
    }
    return client->nickname + "!" + client->username + "@localhost";
}

// Build the RPL_NAMREPLY (353) names list for a channel
static std::string buildNamesList(Server& server, Channel& channel) {
    std::string names;
    for (std::set<int>::iterator it = channel.members.begin(); it != channel.members.end(); ++it) {
        if (!names.empty()) {
            names += " ";
        }
        // prefix operators with @
        if (channel.operators.find(*it) != channel.operators.end()) {
            names += "@";
        }
        names += server.getNickname(*it);
    }
    return names;
}

void handleJoin(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return;
    }
    const std::string& nick = client->nickname;

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.empty()) {
        server.sendReply(fd, "461", nick, "JOIN :Not enough parameters");
        return;
    }

    const std::string& channelName = cmd.arguments[0];

    // Validate channel name
    if (!isValidChannelName(channelName)) {
        // ERR_NOSUCHCHANNEL (403), used both for channel names that don't exist or are invalid
        server.sendReply(fd, "403", nick, channelName + " :No such channel");
        return;
    }

    // Supplied key (if any)
    std::string suppliedKey;
    if (cmd.arguments.size() >= 2) {
        suppliedKey = cmd.arguments[1];
    }

    Channel* channel = server.getChannel(channelName);

    if (channel) {
        // already a member? silently ignore
        if (channel->members.find(fd) != channel->members.end()) {
            return;
        }

        // ERR_INVITEONLYCHAN (473)
        if (channel->inviteOnly) {
            if (channel->invited.find(fd) == channel->invited.end()) {
                server.sendReply(fd, "473", nick, channelName + " :Cannot join channel (+i)");
                return;
            }
        }

        // ERR_BADCHANNELKEY (475)
        if (!channel->key.empty() && suppliedKey != channel->key) {
            server.sendReply(fd, "475", nick, channelName + " :Cannot join channel (+k)");
            return;
        }

        // ERR_CHANNELISFULL (471)
        if (channel->userLimit > 0 && (int)channel->members.size() >= channel->userLimit) {
            server.sendReply(fd, "471", nick, channelName + " :Cannot join channel (+l)");
            return;
        }

        // all checks passed, add member
        channel->members.insert(fd);
        // remove from invite list (one-time use, would need to be reinvited)
        channel->invited.erase(fd);
    } else {
        // create new channel
        // every (registered) user can create a channel, and gets op privileges
        Channel& newChan = server.createChannel(channelName);
        newChan.members.insert(fd);
        newChan.operators.insert(fd); // creator becomes operator
        channel = &newChan;
    }

    // Add user to channel
    client->channels.insert(channelName);

    // Broadcast JOIN to all members (including the joining user)
    std::string joinMsg = ":" + senderPrefix(server, fd) + " JOIN " + channelName;
    server.sendToChannel(channelName, joinMsg, -1); // -1 = nobody excluded

    // Send topic (or RPL_NOTOPIC) to the joining user
    if (!channel->topic.empty()) {
        // RPL_TOPIC (332)
        server.sendReply(fd, "332", nick, channelName + " :" + channel->topic);
    } else {
        // RPL_NOTOPIC (331)
        server.sendReply(fd, "331", nick, channelName + " :No topic is set");
    }

    // When joining a channel, we send these two commands so the IRC client can populate 
    // the list of users in the channel
    // RPL_NAMREPLY (353) + RPL_ENDOFNAMES (366)
    std::string names = buildNamesList(server, *channel);
    // '=' means public channel
    server.sendReply(fd, "353", nick, "= " + channelName + " :" + names);
    server.sendReply(fd, "366", nick, channelName + " :End of /NAMES list");
}
