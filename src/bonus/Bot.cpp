#include "Bot.hpp"

Bot::Bot(const std::string& server, int port, const std::string& password, const std::string& nickname)
    : _botfd(-1), _server(server), _port(port), _password(password), _nickname(nickname), _buffer("") {
}

Bot::Bot(const Bot& other) 
    : _botfd(-1), _server(other._server), _port(other._port), 
      _password(other._password), _nickname(other._nickname), _buffer("") {
    (void)other;
}

Bot& Bot::operator=(const Bot& other) {
    (void)other;
    return *this;
}

Bot::~Bot() {
    if (_botfd != -1) {
        close(_botfd);
    }
}

void Bot::connectToServer(const std::string& server, int port) {
    _botfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_botfd < 0) {
        throw(std::runtime_error("Bot socket creation failed"));
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server.c_str());
    server_addr.sin_port = htons(port);

    if (connect(_botfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            close(_botfd);
            throw(std::runtime_error("Bot connection to server failed"));
        }
    }
    if (fcntl(_botfd, F_SETFL, O_NONBLOCK) < 0) {
        close(_botfd);
        throw(std::runtime_error("Bot set non-blocking failed"));
    }
    std::cout << "Bot connected to server " << server << ":" << port << std::endl;
}

void Bot::authenticate() {
    std::string pass = "PASS " + _password + "\r\n";
    std::string nick = "NICK " + _nickname + "\r\n";
    std::string user = "USER " + _nickname + " 0 * :" + _nickname + " Bot\r\n";
    
    send(_botfd, pass.c_str(), pass.length(), 0);
    send(_botfd, nick.c_str(), nick.length(), 0);
    send(_botfd, user.c_str(), user.length(), 0);
    
    std::cout << "Bot authenticated as " << _nickname << std::endl;
}

void Bot::run() {
    connectToServer(_server, _port);
    authenticate();
    
    std::cout << "Bot running... (Press Ctrl+C to stop)" << std::endl;
    
    while (true) {
        receiveMessage();
        usleep(10000);  // Sleep 10ms to avoid busy waiting
    }
}

void Bot::receiveMessage() {
    char buffer[1024];
    ssize_t bytes = recv(_botfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            throw(std::runtime_error("Bot receive message failed"));
        }
        return;
    } else if (bytes == 0) {
        throw(std::runtime_error("Bot disconnected from server"));
    }
    buffer[bytes] = '\0';
    _buffer += std::string(buffer, bytes);
    size_t pos;
    while ((pos = _buffer.find("\r\n")) != std::string::npos) {
        std::string message = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        handleMessage(message);
    }
}

void Bot::handleMessage(const std::string& message) {
    std::cout << "Server: " << message << std::endl;
    
    // Handle PING - CRITICAL!
    if (message.find("PING") == 0) {
        std::string pong = "PONG " + message.substr(5) + "\r\n";
        send(_botfd, pong.c_str(), pong.length(), 0);
        std::cout << "Responded to PING" << std::endl;
        return;
    }
    
    if (message.find("PRIVMSG") != std::string::npos) {
        
        size_t privmsg_pos = message.find("PRIVMSG ");
        if (privmsg_pos == std::string::npos) return;
        
        size_t target_start = privmsg_pos + 8;
        size_t colon_pos = message.find(" :", target_start);
        std::string target = message.substr(target_start, colon_pos - target_start);
        size_t text_start = colon_pos + 2;
        std::string text = message.substr(text_start);
        
        if (text == "!hello") {
            std::string response = "PRIVMSG " + target + " :Hello! I'm a bot!\r\n";
            send(_botfd, response.c_str(), response.length(), 0);
        }
        else if (text == "!help") {
            std::string response = "PRIVMSG " + target + " :Commands: !hello, !help, !time\r\n";
            send(_botfd, response.c_str(), response.length(), 0);
        }
        
        else if (text == "!time") {
            time_t now = time(0);
            char* dt = ctime(&now);
            std::string timeStr = std::string(dt);
            if (!timeStr.empty() && timeStr[timeStr.length() - 1] == '\n') {
                timeStr.erase(timeStr.length() - 1);
            }
            std::string response = "PRIVMSG " + target + " :Current server time is " + timeStr + "\r\n";
            send(_botfd, response.c_str(), response.length(), 0);
        }

        else if (text.find("!weather") == 0) {
            if (text.length() <= 9 || text[8] != ' ') {
                std::string response = "PRIVMSG " + target + " :Usage: !weather <city>\r\n";
                send(_botfd, response.c_str(), response.length(), 0);
                return;
            }
            std::string city = text.substr(9);
            std::string weather = getWeather(city);
            std::string response = "PRIVMSG " + target + " :Weather in " + city + ": " + weather + "\r\n";
            send(_botfd, response.c_str(), response.length(), 0);
        }
    }
}

void Bot::sendMessage(const std::string& message) {
    std::string msg = message + "\r\n";
    ssize_t bytes_sent = send(_botfd, msg.c_str(), msg.size(), 0);
    if (bytes_sent < 0) {
        throw(std::runtime_error("Bot send message failed"));
    }
}

std::string Bot::getWeather(const std::string& city) {
    std::string command = "curl -s 'wttr.in/" + city + "?format=%C+%t'";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "Unable to get weather";
    }  
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}