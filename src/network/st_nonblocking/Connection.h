#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include "afina/Storage.h"
#include "afina/execute/Command.h"
#include "protocol/Parser.h"
#include "spdlog/logger.h"
#include <cstring>

#include <deque>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps, std::shared_ptr<spdlog::logger> pl) : client_socket(s), pStorage{ps}, _logger{pl}, response_only{false} {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const { return true; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int client_socket;
    struct epoll_event _event;

    bool _is_alive;

    static constexpr int OUTQUE_HIGH = 100;
    static constexpr int OUTQUE_LOW = 90;
    static constexpr int IOVEC_SIZE = 48;
    static constexpr int CLIENT_BUFFER_SIZE = 4096;

    char client_buffer[CLIENT_BUFFER_SIZE];
    std::size_t read_off;

    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;

    std::deque<std::string> responses;
    std::size_t write_off;

    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> pStorage;

    bool response_only;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
