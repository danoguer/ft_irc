#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "../Server.hpp"

// Initialize the command dispatch table on the server.
// This is where all command handlers are registered.
// To add a new command, add a handler file in commands/ and register it here.
void initCommandMap(Server& server);

#endif
