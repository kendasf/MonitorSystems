//
//  timers.h
//  WebGuiPP
//
// Implementation of node/JavaScript like timers interface
//
//  Created by Niksa Orlic on 15/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#ifndef WebGuiPP_timers_h
#define WebGuiPP_timers_h

#include <memory>
#include <vector>
#include <chrono>
#include <ctime>
#include <functional>

namespace gnode {
  
  struct timer_desc {
    timer_desc(std::function<void()> cb, int msecs, bool repeat) :
      cb(cb), msecs(msecs), repeat(repeat), cancelled(false)
    {
      last_tick = std::chrono::steady_clock::now();
    }
    
    bool expired() {
      std::chrono::time_point<std::chrono::steady_clock> crnt_time = std::chrono::steady_clock::now();
      std::chrono::duration<float> duration = crnt_time - last_tick;
      
      if (duration.count() * 1000 < msecs)
        return false;
      
      last_tick = last_tick + std::chrono::duration<int, std::milli>(msecs);
      return true;
    }
    
    std::function<void()> cb;
    bool repeat;
    int msecs;
    bool cancelled;
    
    std::chrono::time_point<std::chrono::steady_clock> last_tick;
  };
  
  typedef std::shared_ptr<timer_desc> timer_desc_ptr;
  
  class timers {
  public:
    static void proc_events() {
      std::vector<timer_desc_ptr> to_erase;
      for(auto i = 0; i < _active_timers.size(); i++) {
        if (_active_timers[i]->cancelled) {
          to_erase.push_back(_active_timers[i]);
          continue;
        }
        
        if (_active_timers[i]->expired()) {
          _active_timers[i]->cb();
          if (!_active_timers[i]->repeat) {
            to_erase.push_back(_active_timers[i]);
          }
        }
      }
      
      for(auto i = 0; i < to_erase.size(); i++) {
        for(auto j = 0; j < _active_timers.size(); j++) {
          if (_active_timers[j] == to_erase[i]) {
            _active_timers.erase(_active_timers.begin() + j);
            break;
          }
        }
      }
    }
    
    static timer_desc_ptr add_timer(timer_desc_ptr tm) {
      _active_timers.push_back(tm);
      return tm;
    }
    
    static void remove_timer(timer_desc_ptr tm) {
      tm->cancelled = true;
    }
    
  private:
    static std::vector<timer_desc_ptr> _active_timers;
  };
  
  
  inline timer_desc_ptr set_interval(std::function<void()> cb, int msecs) {
    return timers::add_timer(timer_desc_ptr(new timer_desc(cb, msecs, true)));
  }
  
  inline void clear_interval(timer_desc_ptr tm) {
    timers::remove_timer(tm);
  }
  
  inline timer_desc_ptr set_timeout(std::function<void()> cb, int msecs) {
    return timers::add_timer(timer_desc_ptr(new timer_desc(cb, msecs, false)));
  }
  
  inline void clear_timeout(timer_desc_ptr tm) {
    timers::remove_timer(tm);
  }

}; //end namespace gnode

#endif
