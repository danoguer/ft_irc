NAME = ircserv
BOT = ircbot

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp Server.cpp Registration.cpp Network.cpp IrcParser.cpp commands/CommandHandler.cpp commands/Privmsg.cpp commands/Pass.cpp commands/Nick.cpp commands/User.cpp commands/Join.cpp commands/Mode.cpp commands/Invite.cpp commands/Kick.cpp commands/Topic.cpp commands/Part.cpp commands/Motd.cpp commands/Cap.cpp commands/Ping.cpp
OBJS = $(SRCS:.cpp=.o)

BOT_SRCS = bot_main.cpp Bot.cpp
BOT_OBJS = $(BOT_SRCS:.cpp=.o)

all: $(NAME) # $(BOT)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(BOT): $(BOT_OBJS)
	$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BOT_OBJS)

fclean: clean
	rm -f $(NAME) $(BOT)

re: fclean all

.PHONY: all clean fclean re
