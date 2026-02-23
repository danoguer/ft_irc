#include "Commands.hpp"
#include "../Server.hpp"

// CAP (IRCv3 capability negotiation) — not implemented.
// We silently ignore the command so clients like irssi that send
// CAP LS before NICK/USER will fall back to legacy registration
// instead of disconnecting on ERR_UNKNOWNCOMMAND.
void handleCap(Server& server, int fd, const IrcCommand& cmd) {
    (void)server;
    (void)fd;
    (void)cmd;
}
