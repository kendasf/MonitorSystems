//
//  path.h
//  WebGuiPP
//
// Implementation of node-like interface to path
//
//  Created by Niksa Orlic on 21/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

namespace gnode {

  class path {
#ifndef WIN32    
    static const char delim = '/';
#else    
    static const char delim = '\\';
#endif    

  public:
    inline static std::string join(const std::string& a, const std::string& b) {
      std::string res;

      res = normalize(a);
      if (res.back() != delim)
        res.push_back(delim);

      if ((b.length() == 1) && (b[0] == '/' || b[0] == '\\'))
        return res;

      normalize_append(b, res);

      return res;
    }
    
    inline static std::string join(const std::string& a, const std::string& b, const std::string& c) {
      std::string res;
      
      res = normalize(a);
      if (res.back() != delim)
        res.push_back(delim);
      
      if ((b.length() == 1) && (b[0] == '/' || b[0] == '\\'))
        return res;
      
      normalize_append(b, res);
      
      if (res.back() != delim)
        res.push_back(delim);
      
      if ((c.length() == 1) && (c[0] == '/' || c[0] == '\\'))
        return res;
      
      normalize_append(c, res);
      
      return res;
    }
    
    inline static std::string join(const std::string& a, const std::string& b, const std::string& c, const std::string& d) {
      std::string res;
      
      res = normalize(a);
      if (res.back() != delim)
        res.push_back(delim);
      
      if ((b.length() == 1) && (b[0] == '/' || b[0] == '\\'))
        return res;
      
      normalize_append(b, res);
      
      if (res.back() != delim)
        res.push_back(delim);
      
      if ((c.length() == 1) && (c[0] == '/' || c[0] == '\\'))
        return res;
      
      normalize_append(c, res);
      
      if (res.back() != delim)
        res.push_back(delim);
      
      if ((d.length() == 1) && (d[0] == '/' || d[0] == '\\'))
        return res;
      
      normalize_append(d, res);
      
      return res;
    }

    inline static std::string dirname(const std::string& a) {
      std::string res;

      for(int i = int(a.length()) - 1; i >= 0; i--) {
        if ((a[i] == '/') || (a[i] == '\\')) {
          return normalize(std::string(a.begin(), a.begin() + i));
        }
      }

      return "";
    }

    inline static std::string dirname_from_normalized(const std::string& a) {
      std::string res;

      for(int i = int(a.length()) - 1; i >= 0; i--) {
        if (a[i] == delim) {
          return std::string(a.begin(), a.begin() + i);
        }
      }

      return "";
    }

    inline static std::string normalize(const std::string& a) {
      std::string r;
      r.reserve(a.length());  

      bool prev_delim = false;
      for(auto i = 0; i < a.length(); i++) {
        if ((a[i] == '/') || (a[i] == '\\')) {
          if (!prev_delim)
            r.push_back(delim);
          prev_delim = true;
        }
        else {
          prev_delim = false;
          r.push_back(a[i]);
        }
      }

      return r;
    }

    inline static void normalize_append(const std::string& a, std::string& r) {
      bool prev_delim = false;
      for(auto i = 0; i < a.length(); i++) {
        if ((a[i] == '/') || (a[i] == '\\')) {
          if (!prev_delim)
            r.push_back(delim);
          prev_delim = true;
        }
        else {
          prev_delim = false;
          r.push_back(a[i]);
        }
      }
    }

  };


}; // end namespace gnode