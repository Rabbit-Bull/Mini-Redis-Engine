#ifndef MINI_REDIS_COMMAND_HPP
#define MINI_REDIS_COMMAND_HPP

#include <iostream>
#include <string>
#include <vector>

namespace mini_redis {

enum class CommandType {
    PING,
    SET,
    GET,
    DEL,
    EXISTS,
    SET_FEN,
    GET_FEN,
    ZADD,
    ZSCORE,
    ZRANGE,
    HELP,
    EXIT,
    UNKNOWN
};

struct Command {
    CommandType type{CommandType::UNKNOWN};
    std::string raw_command_name;
    std::vector<std::string> args;
    bool is_valid{false};
    std::string error_message;
};

inline std::string command_type_to_string(CommandType type) {
    switch (type) {
        case CommandType::PING:    return "PING";
        case CommandType::SET:     return "SET";
        case CommandType::GET:     return "GET";
        case CommandType::DEL:     return "DEL";
        case CommandType::EXISTS:  return "EXISTS";
        case CommandType::SET_FEN: return "SET_FEN";
        case CommandType::GET_FEN: return "GET_FEN";
        case CommandType::ZADD:    return "ZADD";
        case CommandType::ZSCORE:  return "ZSCORE";
        case CommandType::ZRANGE:  return "ZRANGE";
        case CommandType::HELP:    return "HELP";
        case CommandType::EXIT:    return "EXIT";
        case CommandType::UNKNOWN:
        default:                   return "UNKNOWN";
    }
}

} // namespace mini_redis

#endif // MINI_REDIS_COMMAND_HPP
