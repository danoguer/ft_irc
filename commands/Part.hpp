#ifndef PART_HPP
#define PART_HPP

#include "../IrcParser.hpp"

class Server;

void handlePart(Server& server, int fd, const IrcCommand& cmd);

#endif
