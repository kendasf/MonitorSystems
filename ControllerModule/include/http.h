//
//  http.h
//  WebGuiPP
//
// HTTP server module header
//
//  Created by Niksa Orlic on 22/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#ifndef __http_h_included__
#define __http_h_included__

#include "efgy_http.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>

class http {
public:
  struct query {
    std::string pathname;
    mutable std::unordered_map<std::string, std::string> params;

    const std::string& operator[](const std::string& key) const {
      static std::string empty;
      auto it = params.find(key);
      if (it == params.end())
        return empty;
      uudecode(it->second);
      return it->second;
    }
    
    const std::string& operator[](const char* key) const {
      static std::string empty;
      auto it = params.find(key);
      if (it == params.end())
        return empty;
      uudecode(it->second);
      return it->second;
    }
    
  private:
    void uudecode(std::string& uudecode) const {
      bool has_replacements = false;
      std::string replaced;
      for(auto i = 0; i < uudecode.size(); i++) {
        if (!has_replacements && uudecode[i] == '%') {
          if (i > 0)
            replaced = uudecode.substr(0, i - 1);
          has_replacements = true;
        }
        if (uudecode[i] == '%' && (i + 2 < uudecode.length())) {
          char x = uudecode[i + 1] <= '9' ? uudecode[i + 1] - '0' : ::toupper(uudecode[i + 1]) - 'A' + 10;
          x = (x << 4) | (uudecode[i + 2] <= '9' ? uudecode[i + 2] - '0' : ::toupper(uudecode[i + 2]) - 'A' + 10);
          replaced.push_back(x);
          i += 2;
        }
        else if (has_replacements) {
          replaced.push_back(uudecode[i]);
        }
      }
      if (has_replacements)
        std::swap(uudecode, replaced);
    }
  };

  struct replacement {
    int begin;
    int end;
    std::string fname;
    std::vector<std::string> params;
    std::string content;
  };

  static http& inst() {
    static http i;
    return i;
  }

  void start_web_services(const std::string& folder, const std::string& root, const std::string& upload_folder, int port);
  void handle_web_request(efgy::net::http::server<boost::asio::ip::tcp>::session &session, std::string& s, 
    std::map<std::string, std::string, caseInsensitiveLT>& headers);
  void proc_events();

private:
  http() {
    static_folder = "/var/www";
    default_index = "index.html";
    root_path = "/";
  };

  void start_http(int port, const std::string& upload_folder);

  bool handle_includes_calls(std::string& data, const std::string& filepath);

  bool replace_includes(std::string& data, std::vector<replacement>& replacements, bool has_calls);

  static void replace_params(std::string& data, std::vector<std::string>& params);

  void handle_web_service_request(const http::query& qs, 
    const std::map<std::string, std::string, caseInsensitiveLT>& headers,
    efgy::net::http::server<boost::asio::ip::tcp>::session &session);
  

  // TODO:: decrypt

  std::string static_folder;
  std::string default_index;
  std::string root_path;
  boost::asio::io_service io_service;

};

#endif // __http_h_included__
