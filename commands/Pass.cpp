#include "Pass.hpp"
#include "../Server.hpp"

void handlePass(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return;
    }

    // ERR_ALREADYREGISTRED (462) - cannot use PASS after registration
    if (client->registered) {
        server.sendToClient(fd, ":server 462 " + client->nickname + " :You may not reregister");
        return;
    }

    // ERR_NEEDMOREPARAMS (461) - no password provided
    if (cmd.arguments.empty()) {
        server.sendToClient(fd, ":server 461 * PASS :Not enough parameters");
        return;
    }

    // Check password
    if (server.checkPassword(cmd.arguments[0])) {
        client->passAccepted = true;
    } else {
        // ERR_PASSWDMISMATCH (464)
        server.sendToClient(fd, ":server 464 * :Password incorrect");
    }
}
