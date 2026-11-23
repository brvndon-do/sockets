#include "network.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
std::expected<void, std::string> sendAll(const Socket& socket,
                                         std::span<const std::byte> buffer) {
    std::size_t totalSent{};
    while (totalSent < buffer.size()) {
        std::span remainingBuffer{buffer.subspan(totalSent)};
        ssize_t sent{::send(socket.fd(), remainingBuffer.data(),
                            remainingBuffer.size(), MSG_NOSIGNAL)};

        if (sent == -1 && errno == EINTR)
            continue;

        if (sent == -1)
            return std::unexpected{
                std::format("send error: {}", std::strerror(errno))};

        totalSent += static_cast<std::size_t>(sent);
    }

    return {};
}

std::expected<bool, std::string> recvExact(const Socket& socket,
                                           std::span<std::byte> buffer) {
    std::size_t totalReceived{};
    while (totalReceived < buffer.size()) {
        std::span remainingBuffer{buffer.subspan(totalReceived)};
        ssize_t received{::recv(socket.fd(), remainingBuffer.data(),
                                remainingBuffer.size(), 0)};

        if (received == 0 && totalReceived == 0)
            return false;

        if (received == 0 && totalReceived > 0)
            return std::unexpected{"socket closed"};

        if (received == -1 && errno == EINTR)
            continue;

        if (received == -1)
            return std::unexpected{std::format("unexpected socket error: {}",
                                               std::strerror(errno))};

        totalReceived += static_cast<std::size_t>(received);
    }

    return true;
}
} // namespace

Socket::Socket(int fd) : fd_{fd} {}

Socket::~Socket() {
    if (fd_ != -1)
        ::close(fd_);
}

Socket::Socket(Socket&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (fd_ != -1)
            ::close(fd_);

        fd_ = std::exchange(other.fd_, -1);
    }

    return *this;
}

int Socket::fd() const noexcept { return fd_; }

bool Socket::isValid() const noexcept { return fd_ != -1; }

std::expected<void, std::string>
sendMessage(const Socket& socket, std::span<const std::byte> payload) {
    if (payload.size() > UINT16_MAX)
        return std::unexpected{"payload size greater than uint16 max"};

    std::uint16_t header{htons(static_cast<std::uint16_t>(payload.size()))};
    std::vector<std::byte> buffer(sizeof(header) + payload.size());
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::ranges::copy(payload, buffer.begin() + sizeof(header));

    return sendAll(socket, buffer);
}

std::expected<std::optional<std::vector<std::byte>>, std::string>
recvMessage(const Socket& socket) {
    std::array<std::byte, 2> headerBuffer{};
    auto receivedHeader{recvExact(socket, headerBuffer)};
    if (!receivedHeader.has_value())
        return std::unexpected{receivedHeader.error()};

    if (!receivedHeader.value())
        return std::nullopt;

    std::uint16_t headerLength{};
    std::memcpy(&headerLength, headerBuffer.data(), sizeof(headerLength));
    std::uint16_t payloadLength{ntohs(headerLength)};
    std::vector<std::byte> messageBuffer(payloadLength);

    auto receivedMessage{recvExact(socket, messageBuffer)};
    if (!receivedMessage.has_value())
        return std::unexpected{receivedMessage.error()};

    if (!receivedMessage.value())
        return std::unexpected{"disconnected after header was sent"};

    return messageBuffer;
}