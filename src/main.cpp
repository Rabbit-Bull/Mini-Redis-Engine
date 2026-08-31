#include "mini_redis/command.hpp"
#include "mini_redis/engine.hpp"
#include "mini_redis/parser.hpp"
#include "mini_redis/server.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    try {
        mini_redis::DatabaseEngine engine;
        mini_redis::TcpServer server(engine, "127.0.0.1", 6379);
        server.start();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Exception occurred." << std::endl;
        return 1;
    }
    return 0;
}
