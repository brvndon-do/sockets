#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    addrinfo* res{nullptr};
    addrinfo hints{};

    if (argc != 2) {
        std::cerr << "usage: showip hostname\n";
        return EXIT_FAILURE;
    }

    hints.ai_family = AF_UNSPEC;     // IP-version agnostic; supports IPv4/IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (const int status{::getaddrinfo(argv[1], nullptr, &hints, &res)};
        status != 0) {
        std::cerr << std::format("getaddrinfo: {}\n", ::gai_strerror(status));
        return 2;
    }

    std::cout << "IP addresses for " << argv[1] << "\n\n";

    for (auto p{res}; p != nullptr; p = p->ai_next) {
        std::string ipstr;
        if (p->ai_family == AF_INET) {
            // IPv4 branch
            sockaddr_in* ipv4{reinterpret_cast<sockaddr_in*>(p->ai_addr)};
            ipstr.resize_and_overwrite(
                INET_ADDRSTRLEN, [&](char* buf, std::size_t n) {
                    return ::inet_ntop(p->ai_family, &ipv4->sin_addr, buf, n)
                               ? std::strlen(buf)
                               : 0; // failed: leave the string empty
                });
            std::cout << std::format("IPv4: {}\n", ipstr);

        } else {
            // IPv6 branch
            sockaddr_in6* ipv6{reinterpret_cast<sockaddr_in6*>(p->ai_addr)};
            ipstr.resize_and_overwrite(
                INET6_ADDRSTRLEN, [&](char* buf, std::size_t n) {
                    return ::inet_ntop(p->ai_family, &ipv6->sin6_addr, buf, n)
                               ? std::strlen(buf)
                               : 0; // failed: leave the string empty
                });
            std::cout << std::format("IPv6: {}\n", ipstr);
        }
    }

    ::freeaddrinfo(res);

    return 0;
}