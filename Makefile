NAME = ircserv
BOT = ircbot

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Isrc

SRCS = src/main.cpp \
       src/core/Server.cpp \
       src/core/Registration.cpp \
       src/core/Network.cpp \
       src/core/IrcParser.cpp \
       src/commands/CommandUtils.cpp \
       src/commands/Privmsg.cpp \
       src/commands/Pass.cpp \
       src/commands/Nick.cpp \
       src/commands/User.cpp \
       src/commands/Join.cpp \
       src/commands/Mode.cpp \
       src/commands/Invite.cpp \
       src/commands/Kick.cpp \
       src/commands/Topic.cpp \
       src/commands/Part.cpp \
       src/commands/Motd.cpp \
       src/commands/Cap.cpp \
       src/commands/Ping.cpp \
       src/commands/Quit.cpp

OBJDIR = obj
OBJS = $(SRCS:src/%.cpp=$(OBJDIR)/%.o)
BOT_SRCS = src/bonus/Bot.cpp
BOT_OBJS = $(BOT_SRCS:src/bonus/%.cpp=$(OBJDIR)/bonus/%.o)

all: $(OBJDIR) $(NAME)

bonus: $(OBJDIR) $(BOT)

$(OBJDIR):
	mkdir -p $(OBJDIR)/commands $(OBJDIR)/core $(OBJDIR)/bonus

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(BOT): $(BOT_OBJS)
	$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT)

$(OBJDIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/core/%.o: src/core/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/commands/%.o: src/commands/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/bonus/%.o: src/bonus/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME) $(BOT)

re: fclean all

.PHONY: all clean fclean re
