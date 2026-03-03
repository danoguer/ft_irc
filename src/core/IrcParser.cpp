#include "IrcParser.hpp"


static void skipSpaces(const std::string& s, size_t& i) {
    const size_t n = s.size();
    while (i < n && s[i] == ' ') {
        ++i;
    }
}

// Read a space-separated token
static bool readToken(const std::string& s, size_t& i, std::string& out) {
    const size_t n = s.size();
    if (i >= n) {
        return false;
    }
    size_t start = i;
    while (i < n && s[i] != ' ') {
        ++i;
    }
    if (start == i) {
        return false;
    }
    out = s.substr(start, i - start);
    return true;
}

bool parseIrcCommandLine(const std::string& line, IrcCommand& out) {
    out.command.clear();
    out.arguments.clear();

    size_t i = 0;
    const size_t n = line.size();

    skipSpaces(line, i);
    if (i >= n) {
        return false;
    }

    // leading prefixes are not allowed by the spec, and not sent by modern clients,
    // but some legacy clients add them
    // we accept them for compatibility, and discard them
    if (line[i] == ':') {
        std::string ignored;
        if (!readToken(line, i, ignored)) {
            return false;
        }
        skipSpaces(line, i);
        if (i >= n) {
            return false;
        }
    }

    // command
    if (!readToken(line, i, out.command)) {
        return false;
    }

    // params
    while (true) {
        skipSpaces(line, i);
        if (i >= n) {
            break;
        }

        // handle trailing param (only param that may contain spaces and colons,
        // always appears at end of param list, max 1 per command)
        if (line[i] == ':') {
            ++i; // skip ':'
            if (i <= n) {
                out.arguments.push_back(line.substr(i));
            } else {
                out.arguments.push_back(std::string());
            }
            break;
        }

        // handle other params
        std::string param;
        if (!readToken(line, i, param)) {
            break;
        }
        out.arguments.push_back(param);
    }

    return true;
}
