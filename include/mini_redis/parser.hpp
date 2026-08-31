#ifndef MINI_REDIS_PARSER_HPP
#define MINI_REDIS_PARSER_HPP

#include "mini_redis/command.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace mini_redis {

class CommandParser {
public:
    // Tokenizes input string handling quotes, escape characters, and spaces cleanly
    static std::vector<std::string> tokenize(const std::string& line, std::string& error_out);

    // Main parse entry point: turns raw terminal input line into a validated Command struct
    static Command parse(const std::string& line);

private:
    static CommandType parse_command_type(const std::string& token);
    static bool validate_command(Command& cmd);
    static std::string to_upper(std::string str);
};

} // namespace mini_redis

#endif // MINI_REDIS_PARSER_HPP
