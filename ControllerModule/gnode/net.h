//
//  net.h
//  WebGuiPP
//
// Implementation of node-like interface to TCP sockets
//
//  Created by Niksa Orlic on 14/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <memory>
#include <string>
#include <vector>

#include <stdio.h>
#include <cstring>
#include <functional>
#include <errno.h>

#ifndef WebGuiPP_net_h
#define WebGuiPP_net_h

namespace gnode {
  using namespace boost;

  typedef std::shared_ptr<asio::ip::tcp::socket> socket_ptr;

  class connection {
    public:
      connection(socket_ptr socket) : _socket(socket), _total_bytes_in(0) {
        start_read();
      }

      void on_data(std::function<void(const std::string&)> cb) {
        _on_data = cb;
      }

      void on_end(std::function<void()> cb) {
        _on_end = cb;
      }

      void write(const std::string& s) {
        std::shared_ptr<std::string> buffer = std::shared_ptr<std::string>(new std::string(s));
        asio::async_write(*_socket, asio::buffer(*buffer), [&, buffer](const boost::system::error_code &ec, std::size_t length) {});
      }

    private:
      void start_read() {
        asio::async_read(*_socket, _input,
          [&](const boost::system::error_code& error, std::size_t bytes) -> std::size_t { return (bytes == _total_bytes_in) && (!error) ? 1024 : 0; },
          [&](const boost::system::error_code& error, std::size_t bytes){
            if (error) {
              switch(error.value()) {
                case 2:
                  _on_end();
                  for(auto i = 0; i < _connections.size(); i++) {
                    if (_connections[i].get() == this) {
                      _connections.erase(_connections.begin() + i);
                      break;
                    }
                  }
                  return;
              }
            }
            else {
              _total_bytes_in = bytes;
              std::istream is(&_input);
              std::string s;
              s.resize(bytes);
              is.read(&s.front(), bytes);
              _on_data(s);
            }
            start_read();
          }
        );
      }

      socket_ptr _socket;
      std::function<void(const std::string&)> _on_data;
      std::function<void()> _on_end;
      asio::streambuf _input;
      std::size_t _total_bytes_in;
    
  public:
      static std::vector<std::shared_ptr<connection> > _connections;
  };

  typedef std::shared_ptr<connection> connection_ptr;

  class server {
    public:
    server(asio::io_service& io, std::function<void(connection_ptr)> cb) : _io_service(io), _cb(cb) { }

    void listen(int port) {
      _acceptor_ptr = std::shared_ptr<asio::ip::tcp::acceptor>(new asio::ip::tcp::acceptor(_io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)));
      start_accept();
    }

    private:
      void start_accept() {
        socket_ptr socket = socket_ptr(new asio::ip::tcp::socket(_io_service));
        _acceptor_ptr->async_accept(*socket, [socket, this](const boost::system::error_code & error) { on_accept(socket, error); });
      }

      void on_accept(socket_ptr socket, const boost::system::error_code &error) {
        if (!error) {
          connection_ptr conn = connection_ptr(new connection(socket));
          connection::_connections.push_back(conn);
          _cb(conn);
        }
        
        start_accept();
      }

      asio::io_service& _io_service;
      std::shared_ptr<asio::ip::tcp::acceptor> _acceptor_ptr;
      std::function<void(connection_ptr)> _cb;
  };

  typedef std::shared_ptr<server> server_ptr;

  class net {
   static const int max_loop_ms = 2; 

  public:
    inline static void proc_events() {
      std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
      while (_io_service.poll_one() > 0) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > max_loop_ms)
          break;
      }
    }

    static server_ptr create_server(std::function<void(connection_ptr)> cb) {
      return server_ptr(new server(_io_service, cb));
    }
    
  private:
    static asio::io_service _io_service;
  };
  
}; // end namespace gnode


#endif
