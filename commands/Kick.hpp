#ifndef COMMANDS_KICK_HPP
#define COMMANDS_KICK_HPP

#include "../IrcParser.hpp"

class Server;

// KICK: forcefully remove a user from a channel
// Expected args: <channel> <user> [<comment>]
// Only channel operators can kick
// Channel operators can kick each other
void handleKick(Server& server, int fd, const IrcCommand& cmd);

#endif
