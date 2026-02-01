#ifndef COMMANDS_USER_HPP
#define COMMANDS_USER_HPP

#include "../IrcParser.hpp"

class Server;

// USER: set username and realname
// Expected args: <username> <mode> <unused> <realname>
// Example: USER guest 0 * :Real Name
void handleUser(Server& server, int fd, const IrcCommand& cmd);

#endif
