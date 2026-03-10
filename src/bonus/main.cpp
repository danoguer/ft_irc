#include "Bot.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

static bool g_running = true;

void signalHandler(int signal) {
    (void)signal;
    g_running = false;
    std::cout << "\nBot shutting down..." << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <server> <port> <password> [channel]" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 6667 mypassword" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 6667 mypassword #general" << std::endl;
        return 1;
    }

    std::string server = argv[1];
    int port = std::atoi(argv[2]);
    std::string password = argv[3];
    std::string channel = (argc == 5) ? argv[4] : "";
    std::string nickname = "Bot";

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        Bot bot(server, port, password, nickname, channel);
        std::cout << "Starting IRC bot..." << std::endl;
        bot.run(&g_running);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
