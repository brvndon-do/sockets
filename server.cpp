#include "network.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    constexpr int n_backlog{1};

    if (argc != 2) {
        std::cerr << "usage: server [port]\n";
        return EXIT_FAILURE;
    }

    AddrInfoPtr res;
    addrinfo hints{};
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (const int status{
            ::getaddrinfo(nullptr, argv[1], &hints, std::out_ptr(res))};
        status != 0) {
        std::cerr << std::format("getaddrinfo: {}\n", ::gai_strerror(status));
        return EXIT_FAILURE;
    }

    const int yes{1};
    const Socket listener{
        ::socket(res->ai_family, res->ai_socktype, res->ai_protocol)};

    if (const int status{::setsockopt(listener.fd(), SOL_SOCKET, SO_REUSEADDR,
                                      &yes, sizeof(yes))};
        status != 0) {
        std::cerr << std::format("setsockopt: {}\n", std::strerror(errno));
        return EXIT_FAILURE;
    }

    if (const int status{::bind(listener.fd(), res->ai_addr, res->ai_addrlen)};
        status != 0) {
        std::cerr << std::format("bind: {}\n", std::strerror(errno));
        return EXIT_FAILURE;
    }

    if (const int status{::listen(listener.fd(), n_backlog)}; status != 0) {
        std::cerr << std::format("listen: {}\n", std::strerror(errno));
        return EXIT_FAILURE;
    }
    res.reset();

    sockaddr_storage clientAddr{};
    socklen_t clientAddrSize{sizeof(clientAddr)};
    const Socket accepted{::accept(listener.fd(),
                                   reinterpret_cast<sockaddr*>(&clientAddr),
                                   &clientAddrSize)};

    if (!accepted.isValid()) {
        std::cerr << "failed to accept socket\n";
        return EXIT_FAILURE;
    }

    std::cout << "Client connected!\n";

    if (const auto poll{runChat(accepted)}; !poll.has_value()) {
        std::cerr << poll.error() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}