#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "../core/IrcParser.hpp"
#include <string>

class Server;

// All command handlers have the signature:
//   void handleCMD(Server& server, int fd, const IrcCommand& cmd)
//
// They are registered in CommandUtils.cpp and dispatched
// based on the command name.

// Initialize the command dispatch table
void initCommandMap(Server& server);

// Build a ":nick!user@host" prefix string for a client fd
// Returns "*" if the client is unknown
std::string senderPrefix(Server& server, int fd);

// IRC commands
// Check docs/Commands.md for a reference
void handlePass(Server& server, int fd, const IrcCommand& cmd);
void handleNick(Server& server, int fd, const IrcCommand& cmd);
void handleUser(Server& server, int fd, const IrcCommand& cmd);
void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd);
void handleNotice(Server& server, int senderFd, const IrcCommand& cmd);
void handleJoin(Server& server, int fd, const IrcCommand& cmd);
void handlePart(Server& server, int fd, const IrcCommand& cmd);
void handleMode(Server& server, int fd, const IrcCommand& cmd);
void handleInvite(Server& server, int fd, const IrcCommand& cmd);
void handleKick(Server& server, int fd, const IrcCommand& cmd);
void handleTopic(Server& server, int fd, const IrcCommand& cmd);
void handleMotd(Server& server, int fd, const IrcCommand& cmd);
void handleCap(Server& server, int fd, const IrcCommand& cmd);
void handlePing(Server& server, int fd, const IrcCommand& cmd);
void handleQuit(Server& server, int fd, const IrcCommand& cmd);
void handleNotice(Server& server, int senderFd, const IrcCommand& cmd);
void handleJoin(Server& server, int fd, const IrcCommand& cmd);
void handlePart(Server& server, int fd, const IrcCommand& cmd);
void handleMode(Server& server, int fd, const IrcCommand& cmd);
void handleInvite(Server& server, int fd, const IrcCommand& cmd);
void handleKick(Server& server, int fd, const IrcCommand& cmd);
void handleTopic(Server& server, int fd, const IrcCommand& cmd);
void handleMotd(Server& server, int fd, const IrcCommand& cmd);
void handleCap(Server& server, int fd, const IrcCommand& cmd);
void handlePing(Server& server, int fd, const IrcCommand& cmd);
void handleQuit(Server& server, int fd, const IrcCommand& cmd);

#endif
