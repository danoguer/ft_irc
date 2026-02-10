#ifndef COMMANDS_TOPIC_HPP
#define COMMANDS_TOPIC_HPP

#include "../IrcParser.hpp"

class Server;

// TOPIC: view or change the topic of a channel
// Expected args: <channel> [<topic>]
// If mode +t is set, only operators can change the topic
// An empty topic string clears the topic
void handleTopic(Server& server, int fd, const IrcCommand& cmd);

#endif
