#include "network.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: client [host] [port]\n";
        return EXIT_FAILURE;
    }

    AddrInfoPtr res;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (const int status{
            ::getaddrinfo(argv[1], argv[2], &hints, std::out_ptr(res))};
        status != 0) {
        std::cerr << std::format("getaddrinfo: {}\n", ::gai_strerror(status));
        return EXIT_FAILURE;
    }

    Socket conn;

    for (auto p = res.get(); p != nullptr; p = p->ai_next) {
        Socket s{::socket(p->ai_family, p->ai_socktype, p->ai_protocol)};

        if (!s.isValid()) {
            std::cerr << "client: socket\n";
            continue;
        }

        if (const int status{::connect(s.fd(), p->ai_addr, p->ai_addrlen)};
            status != 0) {
            std::cerr << std::format("connect error: {}\n",
                                     std::strerror(errno));
            continue;
        }

        conn = std::move(s);
        break;
    }
    res.reset(); // frees the resource; calls the deleter

    if (!conn.isValid()) {
        std::cerr << "unable to connect\n";
        return EXIT_FAILURE;
    }

    // send down here
    std::string msg{"Hello, from client!"};
    if (const auto sent{sendMessage(conn, std::as_bytes(std::span(msg)))};
        !sent.has_value()) {
        std::cerr << sent.error() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}