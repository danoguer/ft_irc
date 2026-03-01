#include "Commands.hpp"
#include "../core/Server.hpp"

void handleQuit(Server& server, int fd, const IrcCommand& cmd) {
    Client* client = server.getClient(fd);
    if (!client)
        return;

    // Default, if user doesnt send a message
    std::string quitMsg = "Quit";
    // Use their message if is provided
    if (!cmd.arguments.empty()) {
        quitMsg = cmd.arguments[0];
    }
    
    // Needed syntax for the message 
    std::string broadcastMsg = ":" + client->nickname + "!" + 
            client->username + "@localhost QUIT :" + quitMsg;

    // Loop throught the channels and send the message to every channel
    for (std::set<std::string>::iterator it = client->channels.begin();
         it != client->channels.end(); ++it) {
        server.sendToChannel(*it, broadcastMsg, -1);
    }

    // Send an Error message to the client, a must on RFC (RFC 2812)
    server.sendToClient(fd, "ERROR :Closing Link: " + quitMsg);

    // Function which handles the client disconnection
    server.disconnectClient(fd);
}