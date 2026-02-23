#include "Commands.hpp"
#include "../Server.hpp"

// PING <token>: server replies with PONG <servername> :<token>
// Without this, clients disconnect.
void handlePing(Server& server, int fd, const IrcCommand& cmd) {
    std::string token;
    if (!cmd.arguments.empty())
        token = cmd.arguments[0];

    server.sendToClient(fd, ":" + server.getServerName() + " PONG " + server.getServerName() + " :" + token);
}
