#include "mini_redis/server.hpp"
#include <iostream>
#include <sstream>

namespace mini_redis {

TcpServer::TcpServer(DatabaseEngine& engine, const std::string& host, int port)
    : engine_(engine), host_(host), port_(port) {
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed with error code: " << res << std::endl;
    }
#endif
}

TcpServer::~TcpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TcpServer::stop() {
    running_ = false;
    if (listen_socket_ != INVALID_SOCKET) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }
}

void TcpServer::start() {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));
    inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

    if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to " << host_ << ":" << port_ << std::endl;
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket." << std::endl;
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        return;
    }

    running_ = true;
    std::cout << "mini-redis TCP Server listening on " << host_ << ":" << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        socket_t client_socket = accept(listen_socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
            if (!running_) {
                break;
            }
            continue;
        }

        std::thread client_thread(&TcpServer::handle_client, this, client_socket);
        client_thread.detach();
    }
}

void TcpServer::handle_client(socket_t client_socket) {
    char buffer[1024];
    std::string input_accumulator;

    while (running_) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            // Client disconnected or socket closed
            break;
        }

        buffer[bytes_read] = '\0';
        input_accumulator.append(buffer, bytes_read);

        size_t pos;
        while ((pos = input_accumulator.find('\n')) != std::string::npos) {
            std::string line = input_accumulator.substr(0, pos);
            input_accumulator.erase(0, pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) {
                continue;
            }

            Command cmd = CommandParser::parse(line);

            if (cmd.type == CommandType::EXIT) {
                std::string exit_msg = "Exiting mini-redis. Goodbye!\n";
                send(client_socket, exit_msg.c_str(), static_cast<int>(exit_msg.length()), 0);
                closesocket(client_socket);
                return;
            }

            std::string response = engine_.execute(cmd);
            if (!response.empty()) {
                response += "\n";
                send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
            }
        }
    }

    closesocket(client_socket);
}

} // namespace mini_redis
