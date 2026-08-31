#include "mini_redis/repl.hpp"
#include <sstream>

namespace mini_redis {

REPL::REPL(DatabaseEngine& engine, std::istream& in, std::ostream& out)
    : engine_(engine), in_(in), out_(out), running_(true) {}

void REPL::print_prompt() {
    out_ << "mini-redis> " << std::flush;
}

void REPL::print_help() {
    out_ << "mini-redis Supported Commands:\n"
         << "  PING [message]                Echoes 'PONG' or specified message\n"
         << "  SET <key> <value>             Stores key-value pair\n"
         << "  GET <key>                     Retrieves value for key\n"
         << "  DEL <key>                     Deletes key-value pair\n"
         << "  EXISTS <key>                  Checks if key exists\n"
         << "  SET_FEN <key> <fen_string>    Compresses and stores FEN board position\n"
         << "  GET_FEN <key>                 Retrieves and decompresses FEN board position\n"
         << "  ZADD <key> <score> <member>   Adds or updates score for member in sorted set\n"
         << "  ZSCORE <key> <member>         Retrieves score for member in sorted set\n"
         << "  ZRANGE <key> <min> <max>      Retrieves range of members with score in [min, max]\n"
         << "  HELP                          Displays this help message\n"
         << "  EXIT | QUIT                   Exits the REPL\n";
}

void REPL::run() {
    out_ << "Welcome to mini-redis REPL (C++17 Engine v0.1.0)\n"
         << "Type 'HELP' for commands or 'EXIT' to quit.\n\n";

    std::string line;

    while (running_) {
        print_prompt();

        if (!std::getline(in_, line)) {
            // EOF or input stream closed
            out_ << "\nExiting mini-redis. Goodbye!\n";
            break;
        }

        // Clean trailing '\r' if line came from Windows CRLF input
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Fast-path for blank lines
        if (line.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
            continue;
        }

        Command cmd = CommandParser::parse(line);

        if (!cmd.is_valid) {
            if (!cmd.error_message.empty()) {
                out_ << "(error) " << cmd.error_message << "\n";
            }
            continue;
        }

        if (cmd.type == CommandType::EXIT) {
            out_ << "Exiting mini-redis. Goodbye!\n";
            running_ = false;
            break;
        }

        if (cmd.type == CommandType::HELP) {
            print_help();
            continue;
        }

        // Execute command against the database engine and print result
        std::string response = engine_.execute(cmd);
        if (!response.empty()) {
            out_ << response << "\n";
        }
    }
}

} // namespace mini_redis
