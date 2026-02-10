#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "../IrcParser.hpp"

class Server;

// All command handlers have the signature:
//   void handleCMD(Server& server, int fd, const IrcCommand& cmd)
//
// They are registered in CommandHandler.cpp and dispatched
// based on the command name.

// PASS: authenticate with server password
// Expected args: <password>
// Must be sent before NICK/USER
void handlePass(Server& server, int fd, const IrcCommand& cmd);

// NICK: set or change the user's nickname
// Expected args: <nickname>
// Nickname must be 1-9 characters, start with letter/special, and be unique
void handleNick(Server& server, int fd, const IrcCommand& cmd);

// USER: set username and realname (part of registration)
// Expected args: <username> <mode> <unused> <realname>
// Must be sent during initial registration (along with NICK)
void handleUser(Server& server, int fd, const IrcCommand& cmd);

// PRIVMSG: send a message to a user or channel
// Expected args: <target> <text>
// If target begins with #, target is a channel; otherwise it's a nickname
void handlePrivmsg(Server& server, int senderFd, const IrcCommand& cmd);

// JOIN: join a channel (or create it if it doesn't exist)
// Expected args: <channel> [key]
// Channel creator becomes operator
void handleJoin(Server& server, int fd, const IrcCommand& cmd);

// PART: leave a channel
// Expected args: <channel> [reason]
void handlePart(Server& server, int fd, const IrcCommand& cmd);

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

// INVITE: invite a user to a channel
// Expected args: <nickname> <channel>
// If channel is +i (invite-only), only operators can invite
void handleInvite(Server& server, int fd, const IrcCommand& cmd);

// KICK: remove a user from a channel (operators only)
// Expected args: <channel> <nickname> [reason]
void handleKick(Server& server, int fd, const IrcCommand& cmd);

// TOPIC: view or change the channel topic
// Expected args: <channel> [topic]
// If mode +t is set, only operators can change the topic
// An empty topic string clears the topic
void handleTopic(Server& server, int fd, const IrcCommand& cmd);

// MOTD: display the Message of the Day
// No arguments required
void handleMotd(Server& server, int fd, const IrcCommand& cmd);

#endif
