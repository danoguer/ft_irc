#include "User.hpp"
#include "../Server.hpp"

void handleUser(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client) {
        return;
    }

    // ERR_ALREADYREGISTRED (462)
    if (client->registered) {
        server.sendToClient(fd, ":server 462 " + client->nickname + " :You may only register once");
        return;
    }

    // ERR_NEEDMOREPARAMS (461)
    // USER <username> <mode> <unused> <realname>
    if (cmd.arguments.size() < 4) {
        std::string who = client->nickname.empty() ? "*" : client->nickname;
        server.sendToClient(fd, ":server 461 " + who + " USER :Not enough parameters");
        return;
    }

    client->username = cmd.arguments[0];
    // arguments[1] is mode (I THINK we can ignore, we don't implement user modes, TODO check)
    // arguments[2] is unused (typically "*", we accept anything for compatibility)
    client->realname = cmd.arguments[3];

    // try to complete registration
    server.tryCompleteRegistration(fd);
}
