#include "Commands.hpp"
#include "../core/Server.hpp"

#include <sstream>

// Build a mode string like "+itk" from current channel state
static std::string currentModeString(const Channel& ch) {
    std::string modes = "+";
    std::string params;

    if (ch.inviteOnly)
        modes += "i";
    if (ch.topicRestricted)
        modes += "t";
    if (!ch.key.empty()) {
        modes += "k";
        params += " " + ch.key;
    }
    if (ch.userLimit > 0) {
        modes += "l";
        std::ostringstream oss;
        oss << ch.userLimit;
        params += " " + oss.str();
    }

    if (modes == "+")
        return "+"; // no modes set
    return modes + params;
}

void handleMode(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;
    const std::string& nick = client->nickname;

    // ERR_NEEDMOREPARAMS (461)
    if (cmd.arguments.empty()) {
        server.sendReply(fd, "461", nick, "MODE :Not enough parameters");
        return;
    }

    const std::string& target = cmd.arguments[0];

    // We only handle channel modes (target starts with '#')
    // If target doesn't start with '#', it's a user mode — we don't implement those
    if (target.empty() || target[0] != '#') {
        // some clients send user mode queries for every user (MODE <nick>)
        // we don't want to implement those but also don't want to crash,
        // so we accept them and reply with RPL_UMODEIS (221) set to an empty mode
        server.sendReply(fd, "221", nick, "+");
        return;
    }

    // Channel mode
    Channel* channel = server.getChannel(target);
    if (!channel) {
        // ERR_NOSUCHCHANNEL (403)
        server.sendReply(fd, "403", nick, target + " :No such channel");
        return;
    }

    // Query mode (if no mode string given, we just return current modes)
    if (cmd.arguments.size() < 2) {
        // RPL_CHANNELMODEIS (324)
        server.sendReply(fd, "324", nick, target + " " + currentModeString(*channel));
        return;
    }

    // Setting modes, require operator privilege
    if (channel->operators.find(fd) == channel->operators.end()) {
        // ERR_CHANOPRIVSNEEDED (482)
        server.sendReply(fd, "482", nick, target + " :You're not channel operator");
        return;
    }

    const std::string& modeString = cmd.arguments[1];
    bool adding = true; // default to + if no prefix
    size_t paramIdx = 2; // index into cmd.arguments for mode params

    // We'll accumulate applied changes to broadcast
    std::string appliedModes;
    std::string appliedParams;
    bool currentDir = true; // tracks the last direction we appended to appliedModes

    for (size_t i = 0; i < modeString.size(); ++i) {
        char c = modeString[i];

        if (c == '+') {
            adding = true;
            continue;
        }
        if (c == '-') {
            adding = false;
            continue;
        }

        switch (c) {
            case 'i': {
                if (adding != channel->inviteOnly || adding) {
                    channel->inviteOnly = adding;
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += adding ? "+" : "-";
                        currentDir = adding;
                    }
                    appliedModes += "i";
                }
                break;
            }
            case 't': {
                if (adding != channel->topicRestricted || adding) {
                    channel->topicRestricted = adding;
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += adding ? "+" : "-";
                        currentDir = adding;
                    }
                    appliedModes += "t";
                }
                break;
            }
            case 'k': {
                if (adding) {
                    // +k requires a key parameter
                    if (paramIdx >= cmd.arguments.size()) {
                        server.sendReply(fd, "461", nick, "MODE :Not enough parameters");
                        return;
                    }
                    channel->key = cmd.arguments[paramIdx];
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += "+";
                        currentDir = true;
                    }
                    appliedModes += "k";
                    appliedParams += " " + cmd.arguments[paramIdx];
                    ++paramIdx;
                } else {
                    // removing password requires the current key as parameter (RFC 2812)
                    if (paramIdx >= cmd.arguments.size()) {
                        server.sendReply(fd, "461", nick, "MODE :Not enough parameters");
                        return;
                    }
                    channel->key = "";
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += "-";
                        currentDir = false;
                    }
                    appliedModes += "k";
                    appliedParams += " " + cmd.arguments[paramIdx];
                    ++paramIdx;
                }
                break;
            }
            case 'o': {
                // +o/-o requires a nickname parameter
                if (paramIdx >= cmd.arguments.size()) {
                    server.sendReply(fd, "461", nick, "MODE :Not enough parameters");
                    return;
                }
                const std::string& targetNick = cmd.arguments[paramIdx];
                int targetFd = server.findClientFdByNickname(targetNick);
                if (targetFd < 0) {
                    // ERR_NOSUCHNICK (401)
                    server.sendReply(fd, "401", nick, targetNick + " :No such nick/channel");
                    ++paramIdx;
                    break;
                }
                // Target must be in the channel
                if (channel->members.find(targetFd) == channel->members.end()) {
                    // ERR_USERNOTINCHANNEL (441)
                    server.sendReply(fd, "441", nick, targetNick + " " + target + " :They aren't on that channel");
                    ++paramIdx;
                    break;
                }
                if (adding) {
                    channel->operators.insert(targetFd);
                } else {
                    channel->operators.erase(targetFd);
                }
                if (currentDir != adding || appliedModes.empty()) {
                    appliedModes += adding ? "+" : "-";
                    currentDir = adding;
                }
                appliedModes += "o";
                appliedParams += " " + targetNick;
                ++paramIdx;
                break;
            }
            case 'l': {
                if (adding) {
                    // +l requires a limit parameter
                    if (paramIdx >= cmd.arguments.size()) {
                        server.sendReply(fd, "461", nick, "MODE :Not enough parameters");
                        return;
                    }
                    int limit = 0;
                    std::istringstream iss(cmd.arguments[paramIdx]);
                    iss >> limit;
                    if (iss.fail() || limit <= 0) {
                        // ignore invalid limits
                        ++paramIdx;
                        break;
                    }
                    channel->userLimit = limit;
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += "+";
                        currentDir = true;
                    }
                    appliedModes += "l";
                    appliedParams += " " + cmd.arguments[paramIdx];
                    ++paramIdx;
                } else {
                    channel->userLimit = 0;
                    if (currentDir != adding || appliedModes.empty()) {
                        appliedModes += "-";
                        currentDir = false;
                    }
                    appliedModes += "l";
                }
                break;
            }
            default: {
                // ERR_UNKNOWNMODE (472)
                std::string modeChar(1, c);
                server.sendReply(fd, "472", nick, modeChar + " :is unknown mode char to me for " + target);
                break;
            }
        }
    }

    // Broadcast applied mode changes to the channel
    if (!appliedModes.empty()) {
        std::string modeMsg = ":" + senderPrefix(server, fd) + " MODE " + target + " " + appliedModes + appliedParams;
        server.sendToChannel(target, modeMsg, -1); // broadcast to everyone including sender
    }
}
