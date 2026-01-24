#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

#include <string>
#include <vector>

struct IrcCommand {
    std::string command;
    std::vector<std::string> arguments;
};

// Parses a single IRC message
// Expects a complete command, with \r\n removed, in the following format:
//   [:prefix] COMMAND [param1 param2 param3] [:trailing param with spaces]
// Prefixes are allowed for legacy compatibility, but ignored
// Returns true on success, false if the line is empty/invalid
bool parseIrcCommandLine(const std::string& line, IrcCommand& out);

#endif
