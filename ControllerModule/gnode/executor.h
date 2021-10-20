//
//  executor.h
//  WebGuiPP
//
//  Implementation of executor interface
//
//  Created by Niksa Orlic on 09/09/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <memory>
#include <string>
#include <functional>
#include <fcntl.h>
#include <deque>
#include <thread>
#include <mutex>
#include "fs.h"

#ifndef WIN32
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
#endif

#ifndef WebGuiPP_executor_h
#define WebGuiPP_executor_h

namespace gnode {
  typedef std::shared_ptr<std::string> string_ptr;
  
  class executor {
  public:
    
    struct running_process {
      std::string command;
      string_ptr output;
      bool completed;
      bool result;
      std::function<void(bool, string_ptr)> callback;
      
      running_process(const std::string& command, std::function<void(bool, string_ptr)> callback) : command(command), callback(callback), output(string_ptr(new std::string)), completed(false) {}
    };
    
    static void exec(const std::string& command, std::function<void(bool, string_ptr)> cb) {
      _lock.lock();
      _running_processes.push_back(running_process(command, cb));
      _lock.unlock();
    }
    
    static void proc_events() {
      static bool first_run = true;
      if (first_run) {
        new std::thread(executor_thread);
        first_run = false;
      }

      _lock.lock();
      if (!_running_processes.empty() && _running_processes.front().completed) {
          _running_processes.front().callback(_running_processes.front().result, _running_processes.front().output);
          _running_processes.pop_front();
      }
      _lock.unlock();
    }
    
  private:
    static void executor_thread() {
      while(true) {
        _lock.lock();
        if (_running_processes.empty() ||  _running_processes.front().completed) {
          _lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          std::this_thread::yield();
          continue;
        }

        FILE *p = popen(_running_processes.front().command.c_str(), "r");
        if (p == NULL) {
          _running_processes.front().completed = true;
          _running_processes.front().result = false;
          _lock.unlock();
          continue;
        }

        string_ptr output = _running_processes.front().output;

        _lock.unlock();

        char line[128];
        while(fgets(line, 100, p))
          output->append(line);
        pclose(p);


        _lock.lock();
        _running_processes.front().completed = true;
        _running_processes.front().result = true;
        _lock.unlock();
      }
    }

    static std::deque<running_process> _running_processes;
    static std::mutex _lock;
    
  };
  
}; // namespace gnode


#endif
