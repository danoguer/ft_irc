# ft_irc

*This project has been created as part of the 42 curriculum by andfern2 and danoguer.*

## Description

**ft_irc** is a fully functional Internet Relay Chat (IRC) server implementation written in C++98. The project implements the core IRC protocol as defined in RFC 1459, RFC 2810, RFC 2811, RFC 2812, and RFC 2813, allowing multiple clients to connect simultaneously and communicate in real-time through channels and private messages.

The server handles client authentication, registration, channel management (including operators and modes), and various IRC commands. It uses non-blocking I/O with `poll()` to efficiently manage multiple concurrent connections. The implementation focuses on robustness, RFC compliance, and clean architecture with separate layers for networking, parsing, and command execution.

### Key Features

- **Multi-client support**: Handle multiple simultaneous connections using `poll()`
- **Authentication system**: Server password protection via PASS command
- **Channel management**: 
  - Create and join channels with optional passwords
  - Channel operators with special privileges
  - Channel modes: invite-only (+i), topic restrictions (+t), passwords (+k), user limits (+l)
  - Channel operations: KICK, INVITE, TOPIC
- **IRC Commands**: PASS, NICK, USER, JOIN, PART, PRIVMSG, MODE, KICK, INVITE, TOPIC, QUIT, MOTD, CAP, PING
- **Private messaging**: Direct messaging between users
- **MOTD (Message of the Day)**: Custom welcome message for users
- **Bonus feature**: IRC bot with weather information capability

## Instructions

### Requirements

- C++ compiler with C++98 support (g++, clang++)
- Make
- Unix-like operating system (Linux, macOS, WSL)

### Compilation

To compile the IRC server:

```bash
make
```

This will generate the `ircserv` executable.

To compile the bonus IRC bot:

```bash
make bonus
```

This will generate the `ircbot` executable.

To clean object files:

```bash
make clean
```

To remove all compiled files:

```bash
make fclean
```

To recompile everything:

```bash
make re
```

### Execution

#### Starting the Server

```bash
./ircserv <port> <password>
```

**Parameters:**
- `<port>`: The port number on which the server will listen for connections (1024-65535 recommended)
- `<password>`: The connection password that clients must provide

**Example:**
```bash
./ircserv 6667 mySecretPassword
```

#### Connecting with a Client

You can connect to the server using any standard IRC client (e.g., irssi, WeeChat, HexChat, mIRC) or using netcat for testing:

```bash
nc localhost 6667
```

Then send the following commands:
```
PASS mySecretPassword
NICK myNickname
USER myusername 0 * :My Real Name
```

#### Using the Bonus Bot

```bash
# Channel is optional
./ircbot <server> <port> <password> <channel>
```

**Example:**
```bash
./ircbot localhost 6667 mySecretPassword general
```


## Usage Examples

### Basic Client Registration and Channel Operations

```irc
# Authenticate with server
PASS mySecretPassword

# Set your nickname
NICK john

# Register as a user
USER john 0 * :John Doe

# Join a channel
JOIN #general

# Send a message to the channel
PRIVMSG #general :Hello everyone!

# Set channel topic (if you're an operator)
TOPIC #general :Welcome to the general discussion

# Invite someone to an invite-only channel
INVITE alice #private

# Set channel modes
MODE #general +i          # Make channel invite-only
MODE #general +t          # Restrict topic changes to operators
MODE #general +k secret   # Set channel password
MODE #general +l 10       # Set user limit to 10

# Send a private message
PRIVMSG alice :Hey, how are you?

# Leave a channel
PART #general :Goodbye!

# Disconnect from server
QUIT :See you later
```

### Channel Operator Commands

```irc
# Give operator status to a user
MODE #channel +o username

# Remove operator status
MODE #channel -o username

# Kick a user from the channel
KICK #channel baduser :Please follow the rules

# Set channel password
MODE #channel +k mypassword

# Remove channel password
MODE #channel -k
```
### Bot Commands
```irc
# Bot responds with a greeting message
!hello

# Shows the list of available commands
!help

# Returns the current server time
!time

# Gets weather information for a city (uses wttr.in API via curl)
!weather <city>
```

## Project Architecture

The project is organized into three main layers:

1. **Network Layer** (`Network.cpp`): Handles client connections, socket I/O, and message buffering using `poll()`
2. **Parser Layer** (`IrcParser.cpp`): Parses incoming IRC messages into structured command objects
3. **Server Layer** (`Server.cpp`): Maintains server state (clients, channels) and executes commands

### File Structure

```
ft_irc/
├── src/
│   ├── main.cpp                 # Entry point
│   ├── core/                    # Core server components
│   │   ├── Server.cpp/hpp       # Server state and command execution
│   │   ├── Network.cpp/hpp      # Network I/O management
│   │   ├── IrcParser.cpp/hpp    # Message parsing
│   │   └── Registration.cpp     # Client registration logic
│   ├── commands/                # IRC command implementations
│   │   ├── Pass.cpp             # Password authentication
│   │   ├── Nick.cpp             # Nickname management
│   │   ├── User.cpp             # User registration
│   │   ├── Join.cpp             # Channel joining
│   │   ├── Part.cpp             # Channel leaving
│   │   ├── Privmsg.cpp          # Private/channel messages
│   │   ├── Mode.cpp             # Channel/user modes
│   │   ├── Kick.cpp             # Kick users from channels
│   │   ├── Invite.cpp           # Invite users to channels
│   │   ├── Topic.cpp            # Channel topic management
│   │   ├── Quit.cpp             # Client disconnection
│   │   ├── Motd.cpp             # Message of the day
│   │   ├── Cap.cpp              # Capability negotiation
│   │   └── Ping.cpp             # Keep-alive
│   └── bonus/
│       └── Bot.cpp/hpp          # IRC bot implementation
├── config/
│   └── motd.txt                 # Message of the day
├── tests/                       # Test scripts
├── docs/                        # RFC documentation
└── Makefile
```

## Technical Choices

- **C++98 Standard**: Required by 42 curriculum, ensuring compatibility with older systems
- **poll() for I/O Multiplexing**: Efficient handling of multiple client connections without threading
- **Non-blocking Sockets**: Prevents server blocking on slow clients
- **Command Pattern**: Each IRC command has a dedicated handler function
- **State Management**: Centralized server state with clear separation of concerns
- **Message Buffering**: Handles partial messages and ensures complete IRC messages before processing

## Resources

### IRC Protocol Documentation

- [RFC 1459](https://tools.ietf.org/html/rfc1459) - Internet Relay Chat Protocol (original specification)
- [RFC 2810](https://tools.ietf.org/html/rfc2810) - IRC: Architecture
- [RFC 2811](https://tools.ietf.org/html/rfc2811) - IRC: Channel Management
- [RFC 2812](https://tools.ietf.org/html/rfc2812) - IRC: Client Protocol
- [RFC 2813](https://tools.ietf.org/html/rfc2813) - IRC: Server Protocol
- [ircdocs.horse](https://ircdocs.horse/) - Modern IRC Protocol Documentation
- [IRC Numerics List](https://defs.ircdocs.horse/defs/numerics.html) - IRC server reply codes

### Programming References

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - Socket programming in C/C++
- [poll() man page](https://man7.org/linux/man-pages/man2/poll.2.html) - I/O multiplexing documentation
- C++ reference documentation for STL containers (map, set, vector, string)

### Articles and Tutorials

- [How IRC Works](https://www.irchelp.org/protocol/) - IRC protocol overview
- [Building an IRC Server](https://modern.ircdocs.horse/) - Modern IRC implementation guide

### AI Usage

AI assistance was used for the following tasks in this project:

1. **RFC Interpretation**: Understanding the nuances of IRC protocol specifications and edge cases
2. **Code Structure Suggestions**: Reviewing architectural patterns for network servers and command dispatching
3. **Debugging Assistance**: Identifying issues with message parsing, buffer management, and state synchronization
4. **Documentation**: Generating code comments and this README structure
5. **Test Case Generation**: Brainstorming edge cases and test scenarios for IRC command handling

**Scope of AI Use:**
- AI was used as a reference tool and debugging assistant
- All core implementation, architecture decisions, and actual code were written by the project authors
- AI did not write production code directly but helped with understanding concepts and troubleshooting

---
