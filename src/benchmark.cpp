#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace {

class SocketReader {
public:
    explicit SocketReader(socket_t sock) : sock_(sock), head_(0), tail_(0) {}

    bool read_line(std::string& line) {
        line.clear();
        while (true) {
            if (head_ >= tail_) {
                int bytes = recv(sock_, buf_, sizeof(buf_), 0);
                if (bytes <= 0) {
                    return !line.empty();
                }
                head_ = 0;
                tail_ = bytes;
            }

            while (head_ < tail_) {
                char c = buf_[head_++];
                if (c == '\n') {
                    return true;
                }
                if (c != '\r') {
                    line.push_back(c);
                }
            }
        }
    }

private:
    socket_t sock_;
    char buf_[8192];
    int head_{0};
    int tail_{0};
};

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 6379;
    int num_operations = 100000;

    if (argc > 1) {
        num_operations = std::stoi(argv[1]);
    }

#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed with error: " << res << std::endl;
        return 1;
    }
#endif

    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to mini-redis TCP server on " << host << ":" << port << std::endl;
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    SocketReader reader(sock);
    std::string response;

    std::cout << "==================================================" << std::endl;
    std::cout << "       mini-redis Stress Test Benchmarking        " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Target Server   : " << host << ":" << port << std::endl;
    std::cout << "Total Operations: " << num_operations << " SET commands" << std::endl;
    std::cout << "Benchmarking in progress..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_operations; ++i) {
        std::string cmd = "SET bench_key_" + std::to_string(i) + " bench_val_" + std::to_string(i) + "\n";
        send(sock, cmd.c_str(), static_cast<int>(cmd.length()), 0);

        if (!reader.read_line(response)) {
            std::cerr << "Error reading response at operation " << i << std::endl;
            break;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    // Clean exit
    std::string exit_cmd = "EXIT\n";
    send(sock, exit_cmd.c_str(), static_cast<int>(exit_cmd.length()), 0);
    closesocket(sock);

#ifdef _WIN32
    WSACleanup();
#endif

    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    double ops_per_sec = (num_operations / elapsed_ms) * 1000.0;

    std::cout << "\n---------------- Benchmark Results ---------------" << std::endl;
    std::cout << "Completed       : " << num_operations << " operations" << std::endl;
    std::cout << "Total Time      : " << std::fixed << std::setprecision(2) << elapsed_ms << " ms" << std::endl;
    std::cout << "Throughput      : " << std::fixed << std::setprecision(2) << ops_per_sec << " OPS (Operations Per Second)" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
