//
//  http.cpp
//  WebGuiPP
//
// HTTP module implementation
//
//  Created by Niksa Orlic on 22/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include "efgy_http.h"

#include "http.h"
#include "webservices.h"

#include "../gnode/fs.h"
#include "../gnode/path.h"

#include "std_fix.h"
#include <functional>

#include "main.h"

const int max_loop_ms = 4;

std::unordered_map<std::string, std::string> mime_types = {
  { "html", "text/html" },
  { "js", "text/javascript" },
  { "gif", "image/gif" },
  { "png", "image/png" },
  { "css", "text/css" },
  { "eot", "application/vnd.ms-fontobject" },
  { "otf", "application/x-font-opentype" },
  { "svg", "image/svg+xml" },
  { "ttf", "application/x-font-ttf" },
  { "woff", "application/font-woff" },
  { "wav", "audio/wav" },
  { "bin", "application/octet-stream"}
};

using boost::asio::ip::tcp;

static bool on_generic_request(efgy::net::http::server<tcp>::session &session, std::string& s, 
  std::map<std::string, std::string, caseInsensitiveLT>& headers) {
  http::inst().handle_web_request(session, s, headers);
  // printf("Handeled web request\n");
  return true;
}

void http::start_web_services(const std::string& folder, const std::string& root, const std::string& upload_folder, int port) {
  static_folder = folder;
  webservices::inst().start(folder, root, upload_folder);
  root_path = root;

  start_http(port, upload_folder);
  printf("Web server started\n");
}

void http::start_http(int port, const std::string& upload_folder) {
  tcp::resolver resolver(io_service);
    
  boost::asio::ip::tcp::endpoint local_end_point(boost::asio::ip::address::from_string("0.0.0.0"), port);
    
  efgy::net::http::server<tcp> *s = new efgy::net::http::server<tcp>(io_service, local_end_point, upload_folder);
  s->processor.set_handler(on_generic_request);
}

void http::proc_events() {
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  while (io_service.poll_one() > 0) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > max_loop_ms)
      break;
  }
  // printf("Web Events Processed\n");
}

std::string get_extension(const std::string& p) {
  if (p.length() == 0)
    return "";

  int end = -1;
  int start = -1;
  for(int i = int(p.length()) - 1; i >= 0; i--) {
    if ((p[i] == '?') || (p[i] == '#'))
      end = i;
    if ((p[i] == '.') && (start == -1))
      start = i;
    if ((p[i] == '/') || (p[i] == '\\')) {
      if (start == -1)
        return "";

      if ((end == -1) || (end < start))
        return std::string(p.begin() + start + 1, p.end());

      return std::string(p.begin() + start + 1, p.begin() + end);
    }
  }

  return "";
}

http::query parse_query(const std::string& p) {
  http::query res;

  bool pathname = true;
  bool iskey = true;
  std::string key, value;
  for(auto i = 0; i < p.length(); i++) {
    if (pathname) {
      if (p[i] == '?')
        pathname = false;
      else
        res.pathname.push_back(p[i]);
    }
    else {
      if (iskey) {
        if (p[i] == '=')
          iskey = false;
        else
          key.push_back(p[i]);        
      }
      else {
        if (p[i] == '&') {
          iskey = true;
          res.params.insert(std::make_pair(key, value));
          key = "";
          value = "";
        }
        else
          value.push_back(p[i]);
      }
    }
  }

  if (key.length() > 0)
    res.params.insert(std::make_pair(key, value));

  return res;
}


void http::handle_web_request(efgy::net::http::server<tcp>::session &session, std::string& s, 
  std::map<std::string, std::string, caseInsensitiveLT>& headers) {
  
  query qs = parse_query(s);
  
  std::string filepath = gnode::path::join(static_folder, qs.pathname);
  
  if (filepath.length() && (filepath[filepath.length() - 1] == '/' || filepath[filepath.length() - 1] == '\\'))
    filepath += default_index;

  if (!gnode::fs::exists_sync(filepath)) {
    filepath = gnode::path::join(BASE_DATA, qs.pathname);
    if (!gnode::fs::exists_sync(filepath)) {
      handle_web_service_request(qs, headers, session);
      return;
    }
  }
     
  std::string extension = get_extension(filepath);  
  filepath = gnode::path::normalize(filepath);

  std::string data = gnode::fs::read_file_sync(filepath);
  if (filepath.find("DeviceInfoS") != std::string::npos)
    data.clear();
  if (data.length() == 0) {
    session.reply(404, "Content-type: text/plain\r\n", "404 Not found");
    return;
  }

  if (extension == "html") {
    bool has_calls = handle_includes_calls(data, filepath);
    std::string headers = "Content-type: " + mime_types[extension] + "\r\n";
    if (has_calls)
      headers = "Content-type: " + mime_types[extension] + "\r\n" +
        "Cache-Control: no-cache, no-store, must-revalidate\r\n" +
        "Pragma: no-cache\r\n" +
        "Expires: 0\r\n";

    session.reply(200, headers, data);
  }
  else {
    if (mime_types.find(extension) != mime_types.end())
      session.reply(200, "Content-type: " + mime_types[extension] + "\r\n", data);
    else
      session.reply(200, "Content-type: application/octet-stream\r\n", data);
  }
}

bool http::handle_includes_calls(std::string& data, const std::string& filepath) {
  std::string dirname = gnode::path::dirname_from_normalized(filepath);
  
  std::vector<replacement> replacements;
  REGEX_NAMESPACE::smatch includes;
  std::string::const_iterator it = data.begin();

  REGEX_NAMESPACE::regex r("#include\\(([^\\)]*)\\)");
  while(REGEX_NAMESPACE::regex_search(it, data.cend(), includes, r)) {
    it = includes[0].second;

    replacements.push_back(replacement());
    replacements.back().begin = int(includes[0].first - data.begin());
    replacements.back().end = int(includes[0].second - data.begin());

    bool is_fname = true;
    for(auto it = includes[1].first; it != includes[1].second; ++it) {
      if (is_fname) {
        if (*it == ' ') {
          is_fname = false;
          replacements.back().fname = gnode::path::join(dirname, replacements.back().fname);
          replacements.back().params.push_back("");
        }
        else
          replacements.back().fname.push_back(*it);
      }
      else {
        if (*it == ' ') {
          replacements.back().params.push_back("");
        }
        else {
          replacements.back().params.back().push_back(*it);
        }
      }
    }
    
    if (is_fname) {
      replacements.back().fname = gnode::path::join(dirname, replacements.back().fname);
    }

    if (replacements.back().params.size() && replacements.back().params.back().length() == 0) {
      replacements.back().params.erase(replacements.back().params.begin() + replacements.back().params.size() - 1);
    }
    
  }

  bool has_calls = false;
  REGEX_NAMESPACE::smatch calls;
  it = data.begin();
  while(REGEX_NAMESPACE::regex_search(it, data.cend(), calls, REGEX_NAMESPACE::regex("#call\\(([^\\)]*)\\)"))) {
    it = calls[0].second;
    has_calls = true;

    std::string func = calls[1].str();
    replacements.push_back(replacement());
    replacements.back().begin = int(calls[0].first - data.begin());
    replacements.back().end = int(calls[0].second - data.begin());
    replacements.back().content = webservices::inst().ss_call(func);
  }
  
  std::sort(replacements.begin(), replacements.end(), [](const replacement&a , const replacement& b){ return a.begin < b.begin; });

  if (replacements.size() > 0)
    return replace_includes(data, replacements, has_calls);

  return false;
}

bool http::replace_includes(std::string& data, std::vector<replacement>& replacements, bool has_calls) {
  for(int i = int(replacements.size()) - 1; i >= 0 ; i--) {
    if (replacements[i].fname.length() > 0) {
      std::string content = gnode::fs::read_file_sync(replacements[i].fname);
      replace_params(content, replacements[i].params);
      has_calls |= handle_includes_calls(content, replacements[i].fname);
      data.replace(data.begin() + replacements[i].begin, data.begin() + replacements[i].end, content.begin(), content.end());
    }
    else if (replacements[i].content.length() > 0) {
      data.replace(data.begin() + replacements[i].begin, data.begin() + replacements[i].end, replacements[i].content.begin(), replacements[i].content.end());
    }
  }
  return has_calls;
}

void http::replace_params(std::string& data, std::vector<std::string>& params) {
  for(auto i = 0; i < params.size(); i++) {
    auto p = params[i].find("{{");
    if (p != std::string::npos) {
      std::string param = params[i].substr(p);
      int p2;
      while ((p2 = data.find(param)) != std::string::npos) {
        data.replace(p2, param.length(), params[i], 0, p);
      }
    }
  }

  int p3;
  while ((p3 = data.find("{{")) != std::string::npos) {
    auto p2 = data.find("}}");
    if (p2 != std::string::npos) {
      data.replace(p3, p2 - p3 + 2, "");
    }
    else
      break;
  }
}

void http::handle_web_service_request(const http::query& qs, 
  const std::map<std::string, std::string, caseInsensitiveLT>& headers, efgy::net::http::server<tcp>::session &session) {
  auto client_address = session.socket.remote_endpoint().address().to_string();

  webservices::response r = webservices::inst().web_api(qs.pathname, qs, headers, session.get_uploaded_path(), client_address);
  
  std::string response_headers;
  for(auto it = r.headers.begin(); it != r.headers.end(); ++it) {
    response_headers += it->first + ": " + it->second + "\r\n";
  }
  if (r.headers.find("Content-Type") == r.headers.end())
    response_headers += "Content-Type: application/json\r\n";
  
  response_headers += "Access-Control-Allow-Origin: *\r\n";
  response_headers += "Access-Control-Allow-Headers: X-Requested-With, Content-Type\r\n";
    
  session.reply(r.code, response_headers, r.data);
}

