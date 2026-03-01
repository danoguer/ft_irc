#include "Commands.hpp"
#include "../core/Server.hpp"

void handleMotd(Server& server, int fd, const IrcCommand&) {
    server.sendMotd(fd);
}
