//
//  buffer.h
//  WebGuiPP
//
//  Buffer-related utilities.
//
//  Created by Niksa Orlic on 16/09/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <string>

#ifndef WebGuiPP_buffer_h
#define WebGuiPP_buffer_h

namespace gnode {
  inline unsigned int read_uint_32_le(const std::string& s, int pos) {
    if (pos + 4 > s.length())
      return 0;
    
    unsigned int res;
    memcpy(&res, s.c_str() + pos, 4);
    return res;
  }
  
  inline void write_uint_32_le(std::string& s, unsigned int val, int pos) {
    if (pos + 4 > s.length())
      return;
    
    memcpy(&s[0], &val, 4);
  }
  
  inline float read_float_le(const std::string& s, int pos) {
    if (pos + 4 > s.length())
      return 0;
       
    float res;
    memcpy(&res, s.c_str() + pos, 4);
    return res;
  }
  
  inline void write_float_le(std::string& s, float val, int pos) {
    if (pos + 4 > s.length())
      return;
    
    memcpy(&s[0] + pos, &val, 4);
  }
  
  inline short int read_int_16_le(const std::string& s, int pos) {
    if (pos + 2 > s.length())
      return 0;
    
    short int res;
    memcpy(&res, s.c_str() + pos, 2);
    return res;
  }
  
}; //end namespace gnode

#endif
