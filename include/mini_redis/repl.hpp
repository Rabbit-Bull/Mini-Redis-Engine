#ifndef MINI_REDIS_REPL_HPP
#define MINI_REDIS_REPL_HPP

#include "mini_redis/command.hpp"
#include "mini_redis/engine.hpp"
#include "mini_redis/parser.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace mini_redis {

class REPL {
public:
    explicit REPL(DatabaseEngine& engine, std::istream& in = std::cin, std::ostream& out = std::cout);

    // Runs the main interactive REPL loop
    void run();

private:
    DatabaseEngine& engine_;
    std::istream& in_;
    std::ostream& out_;
    bool running_{true};

    void print_prompt();
    void print_help();
};

} // namespace mini_redis

#endif // MINI_REDIS_REPL_HPP
