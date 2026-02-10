#include "Commands.hpp"
#include "../Server.hpp"

void handleMotd(Server& server, int fd, const IrcCommand&) {
    server.sendMotd(fd);
}
