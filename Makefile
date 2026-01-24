NAME = ircserv
BOT = ircbot

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp Server.cpp Network.cpp IrcParser.cpp commands/Privmsg.cpp
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
