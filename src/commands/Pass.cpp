#include "Commands.hpp"
#include "../core/Server.hpp"

void handlePass(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return;
    }

    // ERR_ALREADYREGISTRED (462), cannot use PASS after registration
    if (client->registered) {
        server.sendReply(fd, "462", client->nickname, ":Unauthorized command (already registered)");
        return;
    }

    // ERR_NEEDMOREPARAMS (461), no password provided
    if (cmd.arguments.empty()) {
        server.sendReply(fd, "461", "*", "PASS :Not enough parameters");
        return;
    }

    // Check password
    if (server.checkPassword(cmd.arguments[0])) {
        client->passAccepted = true;
    } else {
        // ERR_PASSWDMISMATCH (464)
        server.sendReply(fd, "464", "*", ":Password incorrect");
    }
}
