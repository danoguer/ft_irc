#ifndef COMMANDS_NICK_HPP
#define COMMANDS_NICK_HPP

#include "../IrcParser.hpp"

class Server;

// NICK: set or change nickname
// Expected args: <nickname>
void handleNick(Server& server, int fd, const IrcCommand& cmd);

#endif
