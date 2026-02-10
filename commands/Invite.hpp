#ifndef COMMANDS_INVITE_HPP
#define COMMANDS_INVITE_HPP

#include "../IrcParser.hpp"

class Server;

// INVITE: invite a user to a channel
// Expected args: <nickname> <channel>
// If channel is invite-only (+i), only operators can invite
// If channel is not invite-only, any member can invite
void handleInvite(Server& server, int fd, const IrcCommand& cmd);

#endif
