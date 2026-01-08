#include "Connection.h"
#include <unistd.h>
#include <cstring>

Connection::Connection(int sockfd, const struct sockaddr_in& addr)
    : fd(sockfd), address(addr), buffer(8192), bytes_read(0), keep_alive(false) {
}

Connection::~Connection() {
    if (fd >= 0) {
        ::close(fd);
    }
}

Connection::Connection(Connection&& other) noexcept
    : fd(other.fd), address(other.address), 
      buffer(std::move(other.buffer)), bytes_read(other.bytes_read),
      keep_alive(other.keep_alive) {
    other.fd = -1;
}

Connection& Connection::operator=(Connection&& other) noexcept {
    if (this != &other) {
        if (fd >= 0) {
            ::close(fd);
        }
        fd = other.fd;
        address = other.address;
        buffer = std::move(other.buffer);
        bytes_read = other.bytes_read;
        keep_alive = other.keep_alive;
        other.fd = -1;
    }
    return *this;
}

void Connection::reset() {
    bytes_read = 0;
    keep_alive = false;
    buffer.clear();
    buffer.resize(8192);
}
