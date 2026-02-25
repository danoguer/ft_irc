#include "Commands.hpp"
#include "../Server.hpp"

std::string senderPrefix(Server& server, int fd) {
    Client* client = server.getClient(fd);
    if (!client)
        return "*";
    return client->nickname + "!" + client->username + "@localhost";
}

void initCommandMap(Server& server) {
    CommandEntry e;

    // Pre-registration commands: registration flow and ping
    e.requiresRegistration = false;

    e.handler = &handleCap;     server.addCommand("CAP",     e);
    e.handler = &handlePass;    server.addCommand("PASS",    e);
    e.handler = &handleNick;    server.addCommand("NICK",    e);
    e.handler = &handleUser;    server.addCommand("USER",    e);

    e.handler = &handlePing;    server.addCommand("PING",    e);

    // Post-registration commands (require completed registration)
    e.requiresRegistration = true;
    
    e.handler = &handlePrivmsg; server.addCommand("PRIVMSG", e);
    e.handler = &handleNotice;  server.addCommand("NOTICE",  e);
    e.handler = &handleJoin;    server.addCommand("JOIN",    e);
    e.handler = &handlePart;    server.addCommand("PART",    e);
    e.handler = &handleMode;    server.addCommand("MODE",    e);
    e.handler = &handleInvite;  server.addCommand("INVITE",  e);
    e.handler = &handleKick;    server.addCommand("KICK",    e);
    e.handler = &handleTopic;   server.addCommand("TOPIC",   e);
    e.handler = &handleMotd;    server.addCommand("MOTD",    e);
}
