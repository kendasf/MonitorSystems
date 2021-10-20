//
//  dgram.h
//  WebGuiPP
//
// Implementation of node-like interface to UDP sockets
//
//  Created by Niksa Orlic on 14/09/14.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <memory>
#include <string>
#include <vector>

#include <stdio.h>
#include <cstring>
#include <functional>
#include <errno.h>

//#define ASIO_STANDALONE
#include <boost/asio.hpp>

#ifndef WebGuiPP_dgram_h
#define WebGuiPP_dgram_h

namespace gnode {
  class socket {
    public:
    socket(asio::io_service& io) : _io_service(io) { }

    void bind(int port) {
      _socket_ptr = std::shared_ptr<asio::ip::udp::socket>(new asio::ip::udp::socket(_io_service, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)));
      start_receive();
    }

    void on_message(std::function<void(const std::string&, const asio::ip::udp::endpoint&)> cb) {
      _on_message = cb;
    }

    private:
      void start_receive() {
        _input.resize(1500);
        _socket_ptr->async_receive_from(
                            asio::buffer(_input),
                            _remote_endpoint, 
                            [&](const boost::system::error_code &ec, std::size_t length) 
                            { 
                              on_receive(ec, length); 
                            }
                          );
      }

      void on_receive(const boost::system::error_code &error, std::size_t bytes) {
        if (!error) {
          if (_on_message) {
            std::string s(_input.begin(), _input.begin() + bytes);
            _on_message(s, _remote_endpoint);
          }
        }
        start_receive();
      }

      asio::io_service& _io_service;
      std::shared_ptr<asio::ip::udp::socket> _socket_ptr;
      asio::ip::udp::endpoint _remote_endpoint;
      std::vector<char> _input;
      std::function<void(const std::string&, const asio::ip::udp::endpoint&)> _on_message;
  };

  typedef std::shared_ptr<gnode::socket> udp_socket_ptr;

  class dgram {
   static const int max_loop_ms = 2; 

  public:
    inline static void proc_events() {
      std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
      while (_io_service.poll_one() > 0) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > max_loop_ms)
          break;
      }
    }

    static udp_socket_ptr create_socket() {
      return udp_socket_ptr(new gnode::socket(_io_service));
    }
    
    static void send(const std::string& s, const std::string& dest, int port) {
      asio::ip::udp::endpoint ep;
      ep.address(asio::ip::address::from_string(dest));
      ep.port(port);
      
      std::shared_ptr<asio::ip::udp::socket> sock = std::shared_ptr<asio::ip::udp::socket>(new asio::ip::udp::socket(_io_service));
      sock->open(asio::ip::udp::v4());
      sock->async_send_to(asio::buffer(s), ep, [sock] (const boost::system::error_code &ec, std::size_t length) { sock->close(); });
    }
    
  private:
    static asio::io_service _io_service;
  };
  
}; // end namespace gnode


#endif
