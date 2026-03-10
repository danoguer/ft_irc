#include "Bot.hpp"

Bot::Bot(const std::string& server, int port, const std::string& password, const std::string& nickname, const std::string& channel)
    : _botfd(-1), _server(server), _port(port), _password(password), _nickname(nickname), _channel(channel), _buffer("") {
    if (!_channel.empty() && _channel[0] != '#') {
        _channel = "#" + _channel;
    }
}

Bot::Bot(const Bot& other) 
    : _botfd(-1), _server(other._server), _port(other._port), 
      _password(other._password), _nickname(other._nickname), _channel(other._channel), _buffer("") {
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
    // Using IPV4(AF_INET) and TCP(SOCK_STREAM)
    _botfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_botfd < 0) {
        throw(std::runtime_error("Bot socket creation failed"));
    }

    // Making the fd to non blocking, essential to handle multiple things
    if (fcntl(_botfd, F_SETFL, O_NONBLOCK) < 0) {
        close(_botfd);
        throw(std::runtime_error("Bot set non-blocking failed"));
    }

    // Struct needed for the connect function
    struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server.c_str()); // Converting server format
    server_addr.sin_port = htons(port); // Changing bit format of port

    //Initiates the connect before anything else
    if (connect(_botfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
            close(_botfd);
            throw(std::runtime_error("Bot connection to server failed"));
        }
    }    
    std::cout << "Bot connected to server " << server << ":" << port << std::endl;
}

// Give the needed string structure for the TCP Handshake
void Bot::authenticate() {
    std::string pass = "PASS " + _password + "\r\n";
    std::string nick = "NICK " + _nickname + "\r\n";
    std::string user = "USER " + _nickname + " 0 * :" + _nickname + " Bot\r\n";
    send(_botfd, pass.c_str(), pass.length(), 0);
    send(_botfd, nick.c_str(), nick.length(), 0);
    send(_botfd, user.c_str(), user.length(), 0);
    std::cout << "Bot authenticated as " << _nickname << std::endl;
}

void Bot::joinChannel() {
    std::string join = "JOIN " + _channel + "\r\n";
    send(_botfd, join.c_str(), join.length(), 0);
    std::cout << "Bot joining channel " << _channel << std::endl;
}


void Bot::run(volatile bool* running) {
    connectToServer(_server, _port);
    authenticate();

    // Wait some time for process all the information
    // It checks the running boolean all the time for Signal Kill interaction
    for (int i = 0; i < 50 && running && *running; ++i) {
        usleep(10000);
    }

    if (!running || !*running) {
        std::cout << "Bot stopped before joining channel." << std::endl;
        return;
    }

    if (!_channel.empty()) {
        joinChannel();
    } else {
        std::cout << "Bot connected without joining a channel. Use !botjoin or /invite to join channels." << std::endl;
    }
    std::cout << "Bot running... (Press Ctrl+C to stop)" << std::endl;

    while (running && *running) {
        receiveMessage();
        // Make the bot much more efficient by not running the loop every milisecond
        usleep(10000);
    }
    std::cout << "Bot stopped gracefully." << std::endl;
}

void Bot::receiveMessage() {
    char buffer[1024];

    // Pulls data from the kernel's internet network buffer into our program's memory
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

    // Since we won't receive always the full data, we are adding everything in other buffer
    _buffer += std::string(buffer, bytes);
    size_t pos;
    // Getting the clean part of the msg and send it to the handleMessage to handle it
    while ((pos = _buffer.find("\r\n")) != std::string::npos) {
        std::string message = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        handleMessage(message);
    }
}

// Makes the format needed for send a msg and send it
void Bot::sendMessage(const std::string& message) {
    std::string msg = message + "\r\n";
    ssize_t bytes_sent = send(_botfd, msg.c_str(), msg.size(), 0);
    if (bytes_sent < 0) {
        throw(std::runtime_error("Bot send message failed"));
    }
}

void Bot::handleMessage(const std::string& message) {
    std::cout << "Server: " << message << std::endl;
    
    // Irc servers send a "PING" every minute, we have to send a "Pong" back.
    // If not the server assumes the connection is dead and kills it. (Ping Timeout).
    if (message.find("PING") == 0) {
        std::string pong = "PONG " + message.substr(5) + "\r\n";
        send(_botfd, pong.c_str(), pong.length(), 0);
        std::cout << "Responded to PING" << std::endl;
        return;
    }

    // If someone uses /invite Bot #channel, the server sends an INVITE message to the BOT
    // We parse it and immediately send a JOIN command
    if (message.find("INVITE") != std::string::npos) {
        size_t invite_pos = message.find("INVITE ");
        if (invite_pos != std::string::npos) {
            size_t channel_start = message.find(" :", invite_pos);
            if (channel_start != std::string::npos) {
                std::string channel = message.substr(channel_start + 2);
                std::string join_cmd = "JOIN " + channel + "\r\n";
                send(_botfd, join_cmd.c_str(), join_cmd.length(), 0);
                std::cout << "Bot invited to and joining channel " << channel << std::endl;
            }
        }
        return;
    }

    // We manage the msg from the people and responses 
    // we read if is in a channel -> we send it the channel
    // or if is a DM -> we send a DM back to him
    if (message.find("PRIVMSG") != std::string::npos) {   
        // 1. Locate the PRIVMSG command
        size_t privmsg_pos = message.find("PRIVMSG ");
        if (privmsg_pos == std::string::npos) return;

        // 2. Extract the SENDER (Nickname)
        // Raw format: :Nickname!Username@Host PRIVMSG ...
        std::string sender = "";
        if (message[0] == ':') {
            size_t exclaim_pos = message.find('!'); // Sender ends at the '!'
            if (exclaim_pos != std::string::npos) {
                sender = message.substr(1, exclaim_pos - 1);
            }
        }

        // 3. Extract the TARGET (Channel or Bot Nickname)
        // The target is between "PRIVMSG " and the " :"
        size_t target_start = privmsg_pos + 8; // Move past "PRIVMSG "
        size_t colon_pos = message.find(" :", target_start); 
        if (colon_pos == std::string::npos) return;

        // Substr from the end of "PRIVMSG " to the start of " :"
        std::string target = message.substr(target_start, colon_pos - target_start);

        // 4. Extract the msg
        // Everything after the " :" is what the user typed
        size_t text_start = colon_pos + 2; // Skip the " :"
        std::string text = message.substr(text_start);

        // 5. Determine where to REPLY
        // Logic: If target is a channel (#42spain), reply to the channel.
        // If target is my bot's name, it's a DM, so reply to the SENDER.
        std::string reply_to = target;
        if (target == _nickname && !sender.empty()) {
            reply_to = sender;
        }

        // Debug output for your terminal
        std::cout << "From: " << sender << " | Target: " << target << " | Text: " << text << std::endl;

        // 6. Respond to specific commands using the Wrapper
        if (text == "!hello") 
            sendMessage("PRIVMSG " + reply_to + " :Hello! I'm a bot!");

        else if (text == "!help")
            sendMessage("PRIVMSG " + reply_to + " :Commands: !hello, !help, !time, !weather <city>");
        
        else if (text == "!time") {
            time_t now = time(0);
            char* dt = ctime(&now);
            std::string timeStr = std::string(dt);
            if (!timeStr.empty() && timeStr[timeStr.length() - 1] == '\n')
                timeStr.erase(timeStr.length() - 1);
            sendMessage("PRIVMSG " + reply_to + " :Current server time is " + timeStr);
        }

        else if (text.find("!weather") == 0) {
            if (text.length() <= 9 || text[8] != ' ') {
                sendMessage("PRIVMSG " + reply_to + " :Usage: !weather <city>");
                return;
            }
            std::string city = text.substr(9);
            std::string weather = getWeather(city);
            sendMessage("PRIVMSG " + reply_to + " :Weather in " + city + ": " + weather);
        }
    }
}

std::string Bot::getWeather(const std::string& city) {
    // 1. Caching Check (1800-second TTL)
    time_t now = time(NULL);
    if (_weatherCache.count(city) && (now - _weatherCache[city].timestamp < 1800)) {
        return _weatherCache[city].data;
    }

    // 2. Security: Sanitize user input to prevent Command Injection!
    // We only allow alphanumeric characters and spaces
    for (size_t i = 0; i < city.length(); ++i) {
        if (!isalnum(city[i]) && city[i] != ' ')
            return "Error: Invalid city name (Security risk)";
    }

    // 3. System Pipe: Call curl to fetch plain-text weather
    // curl transfer data to or from a server
    std::string command = "curl -s --connect-timeout 5 --max-time 8 'wttr.in/" + city + "?format=%C+%t' 2>/dev/null";
    // We create a pipe for the command use and to get the data back which the command give us
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "Unable to fetch weather";

    char buffer[128];
    std::string result = "";
    
    // We are using fgets to reach into the pipe and get the data from the curl call
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe); // Clean up subprocess to avoid zombie processes

    // 4. Final cleaning and Caching
    if (result.empty()) return "Weather service unavailable";

    // Trimmer to make a protocol compliance string.
    while (!result.empty() && (result[result.length()-1] == '\n' || result[result.length()-1] == '\r'))
        result.erase(result.length()-1);

    // We keep in memory the response so we dont need to make other call if we have the info
    WeatherCache cache = {result, now};
    _weatherCache[city] = cache;
    return result;
}