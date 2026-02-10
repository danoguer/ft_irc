#ifndef COMMANDS_MODE_HPP
#define COMMANDS_MODE_HPP

#include "../IrcParser.hpp"

class Server;

// MODE: query or change channel modes
// Supported channel modes:
//   i — invite-only channel
//   t — channel topic can only be set by operators
//   k — channel key (password)
//   o — give/take operator privileges
//   l — set a user limit
// Usage:
//   MODE <channel>                        — query current modes
//   MODE <channel> <+/-modes> [params]    — set/unset modes
void handleMode(Server& server, int fd, const IrcCommand& cmd);

#endif
