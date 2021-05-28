#include "Connection.h"
#include "ServerImpl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() { 
	_is_alive = true;
    read_off = write_off = 0;
    response_only = false;
    _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    responses.clear();
}

// See Connection.h
void Connection::OnError() {
	_is_alive = false;
    _event.events = 0;
}

// See Connection.h
void Connection::OnClose() {
	_is_alive = false;
    _event.events = 0;
}

// See Connection.h
void Connection::DoRead() {
	// Process new connection:
    // - read commands until socket alive
    // - execute each command

	try {
        int readed_bytes = read(client_socket, client_buffer + read_off, sizeof(client_buffer) - read_off);
        if (readed_bytes > 0) 
        {
            _logger->debug("Got {} bytes from socket, {} were before", readed_bytes, read_off);
            readed_bytes += read_off;
            std::size_t parsed_off = 0;
            while (readed_bytes > 0) 
            {
                _logger->debug("Process {} bytes", readed_bytes);
                if (!command_to_execute) 
                {
                    std::size_t parsed = 0;
                    if (parser.Parse(client_buffer + parsed_off, readed_bytes, parsed))
                    {
                    	_logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                    	command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    } else 
                    {
                        _logger->debug("Parse() returned false, parsed = {}", parsed);
                    }
                }
                if (command_to_execute && arg_remains > 0) 
                {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                    std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                    argument_for_command.append(client_buffer + parsed_off, to_read);
                    arg_remains -= to_read;
                    readed_bytes -= to_read;
                    parsed_off += to_read;
                }
                if (command_to_execute && arg_remains == 0) 
                {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) 
                    {
                        argument_for_command.resize(argument_for_command.size() - 2);
                    }
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    result += "\r\n";
                    responses.push_back(result);
                    if (responses.size() >= Connection::OUTQUE_HIGH) 
                    {
                        _event.events &= ~EPOLLIN;
                    }
                    if (!(_event.events & EPOLLOUT)) 
                    {
                        _event.events |= EPOLLOUT;
                    }

                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            }
            if (readed_bytes == 0) 
            {
                read_off = 0;
            }
        }
        else if (readed_bytes == 0) 
        {
			_logger->debug("Connection closed");
            response_only = true;
        } 
        else if (!(errno == EWOULDBLOCK || errno == EAGAIN)) 
        {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    }
    catch (std::runtime_error &ex) 
    {
    	_logger->error("Failed to process connection on descriptor {}: {}", client_socket, ex.what());
        responses.push_back("ERROR\r\n");
        if (!(_event.events & EPOLLOUT)) {
            _event.events |= EPOLLOUT;
        }
        _event.events &= ~EPOLLIN;
        shutdown(client_socket, SHUT_RD);
        response_only = true;
        _logger->debug("Responses only!");
    }
}

// See Connection.h
void Connection::DoWrite() { 
	assert(!responses.empty() && "Write call with empty write buffer");
    std::size_t to_write = 1;
    
    iovec iovecs[IOVEC_SIZE] = {};
    auto it = responses.begin();
    iovecs[0].iov_base = &((*it)[0]) + write_off;
    iovecs[0].iov_len = it->size() - write_off;
    ++it;
    for (; to_write < IOVEC_SIZE && it != responses.end(); ++it, ++to_write) 
    {
        iovecs[to_write].iov_base = &((*it)[0]);
        iovecs[to_write].iov_len = it->size();
    }
    int written_bytes{0};
    if ((written_bytes = writev(client_socket, iovecs, to_write)) > 0) 
    {
        _logger->debug("WRITE   {} {}", responses.size(), written_bytes);
        for (std::size_t i = 0; i < to_write; ++i) 
        {
            if (written_bytes >= iovecs[i].iov_len) 
            {
                written_bytes -= iovecs[i].iov_len;
                responses.pop_front();
            } else 
            {
                break;
            }
        }
        write_off = written_bytes;
    }
    else if (written_bytes < 0 && !(errno == EWOULDBLOCK || errno == EAGAIN)) 
    {
        _is_alive = false;
    }
    if (responses.size() <= Connection::OUTQUE_LOW && !response_only) 
    {
        _event.events |= EPOLLIN;
    }
    if (responses.empty()) 
    {
        _event.events &= ~EPOLLOUT;
    }
    if (response_only && responses.empty()) 
    {
        shutdown(client_socket, SHUT_WR);
    }
    _logger->debug("{} {} {}", responses.size(), _is_alive, write_off);
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
