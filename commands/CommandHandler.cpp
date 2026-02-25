#include "CommandHandler.hpp"
#include "Commands.hpp"

void initCommandMap(Server& server) {
    // Pre-registration commands (allowed before NICK+USER handshake completes)
    CommandEntry e;
    e.requiresRegistration = false;
    e.handler = &handleCap;     server.addCommand("CAP",     e);
    e.handler = &handlePass;    server.addCommand("PASS",    e);
    e.handler = &handleNick;    server.addCommand("NICK",    e);
    e.handler = &handleUser;    server.addCommand("USER",    e);

    // Post-registration commands (require completed registration)
    e.requiresRegistration = true;
    e.handler = &handlePrivmsg; server.addCommand("PRIVMSG", e);
    e.handler = &handleJoin;    server.addCommand("JOIN",    e);
    e.handler = &handlePart;    server.addCommand("PART",    e);
    e.handler = &handleMode;    server.addCommand("MODE",    e);
    e.handler = &handleInvite;  server.addCommand("INVITE",  e);
    e.handler = &handleKick;    server.addCommand("KICK",    e);
    e.handler = &handleTopic;   server.addCommand("TOPIC",   e);
    e.handler = &handleMotd;    server.addCommand("MOTD",    e);
    e.handler = &handleQuit;    server.addCommand("QUIT",    e);
}
