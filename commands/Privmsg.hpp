#ifndef COMMANDS_PRIVMSG_HPP
#define COMMANDS_PRIVMSG_HPP

#include <string>

#include "../IrcParser.hpp"

class Server;

// PRIVMSG: send a message to a user or to a channel
// Expected args: <target> <text>
// If target begins with # target is a whole channel, otherwise it's a nickname
void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd);

#endif
