#ifndef COMMANDS_JOIN_HPP
#define COMMANDS_JOIN_HPP

#include "../IrcParser.hpp"

class Server;

// JOIN: join a channel (or create it if it doesn't exist)
// Expected args: <channel> [<key>]
// Channel names must begin with '#'
// If the channel doesn't exist, the joining user becomes the operator
void handleJoin(Server& server, int fd, const IrcCommand& cmd);

#endif
