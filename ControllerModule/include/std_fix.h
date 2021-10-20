//
//  std_fix.h - fixes std::to_string, std::stoi etc functions that are missing in our cross compiler lib
//  WebGuiPP
//
//  Created by Niksa Orlic on 29/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#ifndef WebGuiPP_std_fix_h
#define WebGuiPP_std_fix_h

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

/* custom std::to_string for float */
template <class T>
inline std::string to_string_digits(T v, int leading, int digits = 0) {
  char buf[64];
  char fmt[32];
  sprintf(fmt, "%%0%dd", leading);
  sprintf(buf, fmt, v);
  return std::string(buf);
};

template <>
inline std::string to_string_digits(float v, int leading, int digits) {
  char buf[64];
  char fmt[32];
  sprintf(fmt, "%%0%d.%df", leading, digits);
  sprintf(buf, fmt, v);
  return std::string(buf);
};

template <>
inline std::string to_string_digits(double v, int leading, int digits) {
  char buf[64];
  char fmt[32];
  sprintf(fmt, "%%0%d.%df", leading, digits);
  sprintf(buf, fmt, v);
  return std::string(buf);
};

inline std::string to_string_hex(int v, int leading) {
  char buf[64];
  char fmt[32];
  sprintf(fmt, "%%0%dX", leading);
  sprintf(buf, fmt, v);
  return std::string(buf);
}




/* std::to_string fix for our cross compiler */
#ifdef TARGET_BUILD

#include <boost/regex.hpp>
#define REGEX_NAMESPACE boost

namespace std
{
  template < typename T >
  inline std::string to_string( const T& v ) {
    std::ostringstream stm ;
    return ( stm << v ) ? stm.str() : "{*** error ***}" ;
  }

  inline int stoi(const std::string &s) {
    return ::atoi(s.c_str());
  }
  
  inline double stod(const std::string &s) {
    return ::atof(s.c_str());
  }

  inline int stoi_over(const std::string &s) {
    return ::atoi(s.c_str());
  }
  
  inline double stod_over(const std::string &s) {
    return ::atof(s.c_str());
  }
}

#define stoi stoi_over
#define stod stod_over

#else
#include <regex>
#define REGEX_NAMESPACE std
#endif

#endif
