//
//  fs.h
//  WebGuiPP
//
// Implementation of node-like interface to filesystem
//
//  Created by Niksa Orlic on 14/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include <memory>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <functional>
#include <errno.h>

#include <aio.h>
#include <unistd.h>
#include <dirent.h>

#define access__ access
#define unlink__ unlink
#define lseek__ lseek
#define read__ read
#define write__ write
#define open__ open
#define close__ close

#ifndef WebGuiPP_fs_h
#define WebGuiPP_fs_h


namespace gnode {
  class file_desc {
  public:
    file_desc(int fd, bool term) : _fd(fd), _term(term) {
    
    }
    
    ~file_desc() { }
    
    inline int fd() const { return _fd; }
    inline bool term() const { return _term; }
    
  private:
    int _fd;
    bool _term;
    
  };
  
  typedef std::shared_ptr<file_desc> file_desc_ptr;
  typedef std::shared_ptr<std::string> string_ptr;
  typedef std::shared_ptr<struct aiocb> aiocb_ptr;


  class fs {
  private:

    struct pending_info {
      pending_info(file_desc_ptr fd, aiocb_ptr a, string_ptr buffer, size_t orig_size, int offset, std::function<void(bool, size_t, string_ptr)> cb) :
        fd(fd), a(a), buffer(buffer), orig_size(orig_size), offset(offset), pending_close(false), cb(cb)
      {}
      
      file_desc_ptr fd;
      aiocb_ptr a;
      string_ptr buffer;
      size_t orig_size;
      int offset;
      bool pending_close;
      std::function<void(bool, size_t, string_ptr)> cb;
    };
    
  public:
    inline static bool proc_events() {
     
      bool r = proc_pending_reads();
      r |= proc_pending_writes();
      return r;

	  return false;
    }
    
    template <typename Callback>
    static void open(const std::string &fname, const std::string& flags, Callback&& cb) {
      file_desc_ptr fd = open_sync(fname, flags);
      bool err = fd.get() ? false : true;
      std::forward<Callback>(cb)(err, fd);
    }
    
    inline static file_desc_ptr open_sync(const std::string &fname, const std::string& flags, bool term = false) {
      int ff = 0;
      if (flags == "r")
        ff = O_RDONLY;
      else if (flags == "w") {
        ff = O_WRONLY;
        if (::access__(fname.c_str(), F_OK) == 0)
          ff |= O_TRUNC;
        else
          ff |= O_CREAT;
      }
      else if (flags == "r+") {
        ff = O_RDWR;
        if (::access__(fname.c_str(), F_OK) == 0)
          ff |= O_TRUNC;
        else
          ff |= O_CREAT;
      }
      else if (flags == "a") {
        ff = O_WRONLY | O_APPEND;
        if (::access__(fname.c_str(), F_OK) != 0)
          ff |= O_CREAT;
      }
      
      int fd = ::open__(fname.c_str(), ff,         
		  S_IRWXU | S_IRWXG | S_IRWXO
      );
      if (fd == -1)
        return file_desc_ptr();
      
      return file_desc_ptr(new file_desc(fd, term));
    }
    
    static void close_sync(file_desc_ptr fd, bool force_kill_pending_ops = false) {

      for(unsigned int i = 0; i < _pending_reads.size(); i++) {
        if (_pending_reads[i].fd == fd) {
          _pending_reads[i].pending_close = true;
          if (force_kill_pending_ops) {
            ::aio_cancel(fd->fd(), NULL);
            close__(fd->fd());
            _pending_reads.erase(_pending_reads.begin() + i);
          }          
          return;
        }
      }
      
      for(unsigned int i = 0; i < _pending_writes.size(); i++) {
        if (_pending_writes[i].fd == fd) {
          _pending_writes[i].pending_close = true;
          if (force_kill_pending_ops) {
            ::aio_cancel(fd->fd(), NULL);              
            close__(fd->fd());
            _pending_writes.erase(_pending_writes.begin() + i);
          } 
          return;
        }
      }
      ::close__(fd->fd());

    }

    inline static bool exists_sync(const std::string &fname) {
      return ::access__(fname.c_str(), F_OK) == 0;
    }
    
    static void read(file_desc_ptr fd, string_ptr buffer, int offset, int length, std::function<void(bool, size_t, string_ptr)> cb) {

      size_t orig_size = buffer->length();
      if (orig_size < (unsigned int)(offset + length))
        buffer->resize(offset + length);
      
      aiocb_ptr a = aiocb_ptr(new struct aiocb());
      a->aio_fildes = fd->fd();
      a->aio_offset = fd->term() ? 0 : ::lseek__(fd->fd(), 0, SEEK_CUR);
      a->aio_nbytes = length;
      a->aio_buf = &buffer->front() + offset;
      int err = ::aio_read(a.get());
      
      if (err != 0) {
        buffer->resize(orig_size);
        cb(true, 0, buffer);
        return;
      }
      
      _pending_reads.push_back(pending_info(fd, a, buffer, orig_size, offset, cb));     
    }
    
    static size_t read_sync(file_desc_ptr fd, std::string& buffer, int offset, size_t length) {
      size_t orig_size = buffer.length();
      if (orig_size < (size_t)(offset + length) )
        buffer.resize(offset + length);
      ssize_t bytes_read = ::read__(fd->fd(), &buffer.front() + offset, length);
      
      if (bytes_read == -1) {
        buffer.resize(orig_size);
        return 0;
      }
      
      if (orig_size < (size_t)(offset + bytes_read))
        buffer.resize(offset + bytes_read);
      
      return bytes_read;
    }
    
    static void write(file_desc_ptr fd, string_ptr buffer, std::function<void(bool, size_t, string_ptr)> cb) {
     
      struct stat s;
      ::fstat(fd->fd(), &s);
      
      aiocb_ptr a = aiocb_ptr(new struct aiocb());
      a->aio_fildes = fd->fd();
      a->aio_offset = ::lseek__(fd->fd(), 0, SEEK_CUR);
      a->aio_nbytes = buffer->size();
      a->aio_buf = &buffer->front();
      int err = ::aio_write(a.get());
      
      if (err != 0) {
        cb(true, 0, buffer);
        return;
      }
      
      _pending_writes.push_back(pending_info(fd, a, buffer, 0, 0, cb));
   
    }
    
    static size_t write_sync(file_desc_ptr fd, std::string& buffer) {
      size_t bytes_written = ::write__(fd->fd(), buffer.c_str(), buffer.size());
      return bytes_written;
    }
    
    inline static void write_file_sync(const std::string& fname, const std::string& buffer) {
      int ff = O_WRONLY;
      if (::access__(fname.c_str(), F_OK) == 0)
        ff |= O_TRUNC;
      else
        ff |= O_CREAT;
      
      int fd = ::open__(fname.c_str(), ff,         
		  S_IRWXU | S_IRWXG | S_IRWXO

      );
      if (fd == -1)
        return;
      
      ::write__(fd, buffer.c_str(), buffer.size());
      ::close__(fd);
    }
    
    inline static std::string read_file_sync(const std::string& fname) {
      int fd = ::open__(fname.c_str(), O_RDONLY);
      if (fd == -1)
        return "";
      
      struct stat s;
      ::fstat(fd, &s);
      
      size_t size_to_read = s.st_size;
      if (size_to_read == 0)
        size_to_read = 1024;
      
      std::string data;
      data.resize(size_to_read);
      
      size_t bytes_read = ::read__(fd, &data.front(), size_to_read);
      ::close__(fd);
      
      if (bytes_read <= 0)
        return "";
      
      data.resize(bytes_read);
      return data;
    }
    
    static void unlink_sync(const std::string& fname) {
      ::unlink__(fname.c_str());
    }
    
    static void rename_sync(const std::string& old_fname, const std::string& new_fname) {
      ::rename(old_fname.c_str(), new_fname.c_str());
    }
    
    static std::vector<std::string> readdir_sync(const std::string& folder) {
      DIR           *d;
      struct dirent *dir;
      
      std::vector<std::string> fnames;
      
      d = ::opendir(folder.c_str());
      if (d) {
        while ((dir = ::readdir(d)) != NULL) {
          if ((strcmp(dir->d_name, ".") != 0) && (strcmp(dir->d_name, "..") != 0))
            fnames.push_back(dir->d_name);
        }
      }
      
      ::closedir(d);
      
      return fnames;
    }
    
    static void fsync_sync(file_desc_ptr fd) {
      fsync(fd->fd());
    }
    
  private:
    static bool proc_pending_reads() {
      bool r = false;
      for(unsigned int i = 0; i < _pending_reads.size(); i++) {
        if ( ::aio_error(_pending_reads[i].a.get()) == ECANCELED) {
          if (_pending_reads[i].pending_close)
            ::close__(_pending_reads[i].fd->fd());
          
          _pending_reads[i].buffer->resize(_pending_reads[i].orig_size);
          _pending_reads[i].cb(true, 0, _pending_reads[i].buffer);
          _pending_reads.erase(_pending_reads.begin() + i);
          --i;
          continue;
        }
        
        if ( ::aio_error(_pending_reads[i].a.get()) == 0) {
          if (_pending_reads[i].pending_close)
            ::close__(_pending_reads[i].fd->fd());
          
          ssize_t bytes_read = ::aio_return(_pending_reads[i].a.get());
          if (bytes_read == -1) {
            _pending_reads[i].buffer->resize(_pending_reads[i].orig_size);
          }
          else {
            if (_pending_reads[i].orig_size < (size_t)(_pending_reads[i].offset) + (size_t)bytes_read)
              _pending_reads[i].buffer->resize(_pending_reads[i].offset + bytes_read);
            ::lseek__(_pending_reads[i].fd->fd(), bytes_read, SEEK_CUR);
          }
          
          _pending_reads[i].cb(bytes_read == -1, bytes_read != -1 ? bytes_read : 0, _pending_reads[i].buffer);
          _pending_reads.erase(_pending_reads.begin() + i);
          --i;
          r = true;
          continue;
        }
      }

      return r;
    }
    
    static bool proc_pending_writes() {
      bool r = false;
      for(unsigned int i = 0; i < _pending_writes.size(); i++) {
        if ( ::aio_error(_pending_writes[i].a.get()) == ECANCELED) {
          if (_pending_writes[i].pending_close)
            ::close__(_pending_writes[i].fd->fd());
          
          _pending_writes[i].cb(true, 0, _pending_writes[i].buffer);
          _pending_writes.erase(_pending_writes.begin() + i);
          --i;
          continue;
        }
        
        if ( ::aio_error(_pending_writes[i].a.get()) == 0) {
          if (_pending_writes[i].pending_close)
            ::close__(_pending_writes[i].fd->fd());
          
          ssize_t bytes_written = ::aio_return(_pending_writes[i].a.get());
          if (bytes_written != -1)
            ::lseek__(_pending_writes[i].fd->fd(), bytes_written, SEEK_CUR);
          _pending_writes[i].cb(bytes_written != -1, bytes_written != -1 ? bytes_written : 0, _pending_writes[i].buffer);
          _pending_writes.erase(_pending_writes.begin() + i);
          --i;
          r = true;
          continue;
        }
      }
      return r;
    }
    
    
    static std::vector<pending_info> _pending_reads;
    static std::vector<pending_info> _pending_writes;
   
  };
  
}; // end namespace gnode
#endif