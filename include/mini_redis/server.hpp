#ifndef MINI_REDIS_SERVER_HPP
#define MINI_REDIS_SERVER_HPP

#include "mini_redis/engine.hpp"
#include "mini_redis/parser.hpp"
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace mini_redis {

class TcpServer {
public:
    explicit TcpServer(DatabaseEngine& engine, const std::string& host = "127.0.0.1", int port = 6379);
    ~TcpServer();

    // Non-copyable
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // Starts listening and accepting TCP connections
    void start();

    // Stops the TCP server and closes listening socket
    void stop();

private:
    DatabaseEngine& engine_;
    std::string host_;
    int port_;
    socket_t listen_socket_{INVALID_SOCKET};
    bool running_{false};

    void handle_client(socket_t client_socket);
};

} // namespace mini_redis

#endif // MINI_REDIS_SERVER_HPP
