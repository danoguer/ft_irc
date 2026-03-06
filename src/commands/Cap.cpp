#include "Commands.hpp"
#include "../core/Server.hpp"

// CAP (IRCv3 capability negotiation). 
// We don't support any IRCv3 capabilities.
// According to the standard, we should not implement this command.
// However, if the command isn't implemented we'd return ERR_UNKNOWNCOMMAND,
// and some clients (like irssi) disconnect if we do this.
// For compatibility, we implement and silently ignore the CAP command.
void handleCap(Server& server, int fd, const IrcCommand& cmd) {
    (void)server;
    (void)fd;
    (void)cmd;
}
