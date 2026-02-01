#ifndef COMMANDS_PASS_HPP
#define COMMANDS_PASS_HPP

#include "../IrcParser.hpp"

class Server;

// PASS: authenticate with server password
// Expected args: <password>
// Must be sent before NICK/USER
void handlePass(Server& server, int fd, const IrcCommand& cmd);

#endif
