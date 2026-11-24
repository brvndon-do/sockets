#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <netdb.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

// NOTE: originally I used a function pointer, but this costs a little more
// since unique_ptr has to store the function pointer, making it 2 pointers
// wide. each deletion calls through that stored pointer, so there is
// indirection and the compiler cannot inline.
using AddrInfoPtr =
    std::unique_ptr<addrinfo, decltype([](addrinfo* p) { ::freeaddrinfo(p); })>;

class Socket {
private:
    int fd_{-1};

public:
    Socket() = default;
    explicit Socket(int fd);

    // copy
    Socket(const Socket& other) = delete;
    Socket& operator=(const Socket& other) = delete;

    // move
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    ~Socket();

    [[nodiscard]] int fd() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
};

[[nodiscard]] std::expected<void, std::string>
sendMessage(const Socket& socket, std::span<const std::byte> payload);

[[nodiscard]] std::expected<std::optional<std::vector<std::byte>>, std::string>
recvMessage(const Socket& socket);

// NOTE: not really network-related, but network.hpp is shared so whatever.
[[nodiscard]] std::expected<void, std::string> runChat(const Socket& socket);