#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>

/**
 * Gère une connexion client
 */
class Connection {
public:
    int fd;
    struct sockaddr_in address;
    std::vector<char> buffer;
    size_t bytes_read;
    bool keep_alive;

    Connection(int sockfd, const struct sockaddr_in& addr);
    ~Connection();

    // Non-copyable
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    // Movable
    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    void reset();
};
