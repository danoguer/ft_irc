#include "core/Server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <port> [password]" << std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);
    std::string password = (argc == 3) ? argv[2] : "";
    try {
        Server server(port, password);
        std::cout << "Starting IRC server on port " << port << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
