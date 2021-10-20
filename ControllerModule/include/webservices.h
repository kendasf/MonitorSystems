//
//  webservices.h
//  WebGuiPP
//
//  Webservices module header
//
//  Created by Niksa Orlic on 25/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <unordered_map>

#include "http.h"

class webservices {
  struct auth_token {
    std::time_t expiry;
    std::string token;
    std::string client_ip;
    std::string user_agent;
    int level;
  };

  typedef std::map<std::string, std::string, caseInsensitiveLT> header_map;

public:
  struct response {
    int code;
    std::string data;
    header_map headers;
    
    response() : code(200), headers() {}
    response(const std::string& data) : code(200), data(data) {}
    response(const std::string& data, int code) : code(code), data(data) {}
  };
  
  static webservices& inst() {
    static webservices i;
    return i;
  }

  void start(const std::string& folder, const std::string& rfolder, const std::string& upload_folder);
  response web_api(const std::string& func, const http::query& qs,
                   const std::map<std::string, std::string, caseInsensitiveLT>& headers, const std::string& uploaded_path, const std::string&);
  std::string ss_call(const std::string& func);

private:
  webservices(); 

  void get_system_uptime();
  void cleanup_auth_tokens();
  
  response make_redirect(const std::string& referer);

  response css_theme(const http::query& qs, const header_map& headers, const std::string&, const std::string&);

  response get_salt(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_auth_token(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response test_token(const http::query& qs, const header_map& headers, const std::string&, const std::string&);

  response get_status(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response change_param(const http::query& qs, const header_map& headers, const std::string&, const std::string&);

  response list_folder(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response delete_file(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_bitmap_data(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response enum_library_contents(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_library_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_library_bitmap_data(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response save_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_dimming_table(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response start_test_mode(const http::query& qs, const header_map& headers, const std::string&, const std::string&);  
  response get_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string&);  
  response delete_schedule_entry(const http::query& qs, const header_map& headers, const std::string&, const std::string&);  
  response add_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string&);  
  response activate_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_panels_config(const http::query& qs, const header_map& headers, const std::string&, const std::string&);  
  response set_panels_config(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response upload(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response get_bvs_data(const http::query& qs, const header_map& headers, const std::string&, const std::string&);
  response radar_autoconf(const http::query& qs, const header_map& headers, const std::string& path, const std::string& client_ip);

  response take_snapshot(const http::query& qs, const header_map& headers, const std::string&, const std::string&); 
  response get_last_snapshot(const http::query& qs, const header_map& headers, const std::string&, const std::string&); 
  
  int check_auth(const http::query& qs, const header_map& headers, const std::string&);
  int update_auth_expiry(const http::query& qs, const header_map& headers, const std::string&);
  
  template <class T>
  void add_web_api(const std::string& n, T f);
  
  template <class T>
  void add_ss_call(const std::string& n, T f);
  
  bool extract_uploaded_file_content(const std::string& infile, const std::string& outfile);
  bool decrypt_uploaded_file(const std::string& infile, const std::string& outfile);

  void clear_expired_salts();
  
  int uptime;
  int system_uptime;
  std::string static_folder;
  std::unordered_map<std::string, std::function<response(const http::query& qs, const header_map& headers, const std::string&, const std::string&)> > web_apis;
  std::unordered_map<std::string, std::function<std::string(void)> > ss_calls;
  std::string _root_folder;
  std::string _upload_folder;

  std::unordered_map<int, std::pair<std::string, std::time_t>> _salts;
  std::vector<auth_token> _auth_tokens;
};
