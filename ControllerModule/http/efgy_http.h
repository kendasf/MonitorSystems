/**\file
 * \brief asio.hpp HTTP Server
 *
 * An asynchronous HTTP server implementation using asio.hpp and std::regex.
 *
 * \copyright
 * Copyright (c) 2012-2015, ef.gy Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/libefgy
 * \see Project Source Code: https://github.com/ef-gy/libefgy
 */

#if !defined(EF_GY_HTTP_H)
#define EF_GY_HTTP_H

#include <map>

#include "std_fix.h"
#include <system_error>
#include <algorithm>
#include <functional>
#include <algorithm>

#include "efgy_server.h"

#ifdef WIN32
#define unlink__ _unlink
#else
#define unlink__ unlink
#endif

class caseInsensitiveLT
    : private std::binary_function<std::string, std::string, bool> {
public:
  /**\brief Case-insensitive string comparison
   *
   * Compares two strings case-insensitively.
   *
   * \param[in] a The first of the two strings to compare.
   * \param[in] b The second of the two strings to compare.
   *
   * \returns 'true' if the first string is "less than" the second.
   */
  bool operator()(const std::string &a, const std::string &b) const {
    return std::lexicographical_compare(
        a.begin(), a.end(), b.begin(), b.end(),
        [](const unsigned char & c1, const unsigned char & c2)->bool {
      return tolower(c1) < tolower(c2);
    });
  }
};

namespace efgy {
namespace net {
/**\brief HTTP handling
 *
 * Contains an HTTP server and templates for session management and
 * processing by user code.
 */
namespace http {

template <typename base, typename requestProcessor> class session;

/**\brief HTTP processors
 *
 * This namespace is reserved for HTTP "processors", which contain the logic to
 * actually handle an HTTP request after it has been parsed.
 */
namespace processor {
    using namespace boost;
/**\brief Base processor
 *
 * This is the default processor, which fans out incoming requests by means of a
 * set of regular expressions. If a regex matches, a corresponding function is
 * called, which gets the session and the regex match results.
 *
 * If no regex should match, a 404 response is generated.
 *
 * \note If you need to keep track of custom, per-server data, then the best way
 *       to do so would probably involve extending this object and adding the
 *       data you need. A reference to the processor is available via the
 *       the session object, and it is kept around for as long as the server
 *       object is.
 *
 * \tparam sock The socket class for the request, e.g. asio::ip::tcp
 */
template <class sock> class base {
public:
  /**\brief Session type
   *
   * This is the session type that the processor is intended for. This typedef
   * is mostly here for convenience.
   */
  typedef session<sock, base<sock> > session_spec;

  /**\brief Handle request
   *
   * This is the generic inbound request handler. Whenever a new request needs
   * to be handled, this will go through the registered list of regexen, and for
   * all that match it will call the registered function, until one of them
   * returns true.
   *
   * \param[out] sess The session object where the request was made.
   *
   * \returns true (if successful; but always, really).
   */
  bool operator()(session_spec &sess, std::map<std::string, std::string, caseInsensitiveLT>& headers) {
    return processor(sess, sess.resource, headers);
  }

  /**\brief Add handler
   *
   * This function adds a handler for a specific regex. Currently this is stored
   * in a map, so the order is probably unpredictable - but also probably just
   * alphabetic.
   *
   * \param[in]  rx      The regex that should trigger a given handler.
   * \param[out] handler The function to call.
   *
   * \returns A reference to *this, so you can chain calls.
   */

  base &set_handler(std::function<bool(session_spec &, std::string &, std::map<std::string, std::string, caseInsensitiveLT>&)> handler) {
    processor = handler;
    return *this;
  }

protected:
  /**\brief Map of request handlers
   *
   * This is the map that holds the request handlers. It maps regex strings to
   * handler functions, which is fairly straightforward.
   */
  std::function<bool(session_spec &, std::string &, std::map<std::string, std::string, caseInsensitiveLT>&)> processor;
};
}

template <typename base, typename requestProcessor = processor::base<base> >
using server = net::server<base, requestProcessor, session>;

/**\brief Session wrapper
 *
 * Used by the server to keep track of all the data associated with a single,
 * asynchronous client connection.
 *
 * \tparam requestProcessor The functor class to handle requests.
 */
template <typename base, typename requestProcessor> class session {
protected:
  /**\brief Server type
   *
   * This is the type of the server that the session is being served by; used
   * when instantiating a session, as we need to use some of the data the server
   * has to offer.
   */
  typedef server<base, requestProcessor> serverType;

  /**\brief HTTP request parser status
   *
   * Contains the current status of the request parser for the current session.
   */
  enum {
    stRequest,    /**< Request received. */
    stHeader,     /**< Currently parsing the request header. */
    stContent,    /**< Currently parsing the request body. */
    stProcessing, /**< Currently processing the request. */
    stErrorContentTooLarge,
    /**< Error: Content-Length is greater than
     * maxContentLength */
    stShutdown /**< Will shut down the connection now. Set in the destructor. */
  } status;

  /**\brief Case-insensitive comparison functor
   *
   * A simple functor used by the attribute map to compare strings without
   * caring for the letter case.
   */

public:
  std::shared_ptr<session> self;

  /**\brief Stream socket
   *
   * This is the asynchronous I/O socket that is used to communicate with the
   * client.
   */
  typename base::socket socket;

  /**\brief The request's HTTP method
   *
   * Contains the HTTP method that was used in the request, e.g. GET, HEAD,
   * POST, PUT, etc.
   */
  std::string method;

  /**\brief Requested HTTP resource location
   *
   * This is the location that was requested, e.g. / or /fortune or something
   * like that.
   */
  std::string resource;

  /**\brief HTTP request headers
   *
   * Contains all the headers that were sent along with the request. Things
   * like Host: or DNT:. Uses a special case-insensitive sorting function for
   * the map, so that you can query the contents without having to know the case
   * of the headers as they were sent.
   */
  std::map<std::string, std::string, caseInsensitiveLT> header;

  /**\brief HTTP request body
   *
   * Contains the request body, if the request contained one, and it was not saved as a file.
   */
  std::string content;

  /**\brief Server instance
   *
   * A reference to the server that this session belongs to and was spawned
   * from. Used to process requests and potentially for general maintenance.
   */
  serverType &the_server;

  /**\brief Maximum request content size
   *
   * The maximum number of octets supported for a request body. Requests larger
   * than this are cancelled with an error.
   */
  std::size_t maxContentLength = (1024 * 1024 * 5);

  /**\brief Construct with I/O service
   *
   * Constructs a session with the given asynchronous I/O service.
   *
   * \param[in]  pServer The server instance this session belongs to.
   */
  session(serverType &pServer)
      : self(this), the_server(pServer), socket(pServer.io), status(stRequest),
  input(new boost::asio::streambuf()) {}

  /**\brief Destructor
   *
   * Closes the socket, cancels all remaining requests and sets the status to
   * stShutdown.
   */
  ~session(void) {
    status = stShutdown;
    // try {
    //   socket.shutdown(base::socket::shutdown_both);
    // }
    // catch (std::system_error & e) {
    // }

    try {
      socket.close();
    }
    catch (std::system_error & e) {
    }
    
    if (!uploadedFilePath.empty()) {
      ::unlink__(uploadedFilePath.c_str());
    }
    
    // delete input;
  }

  /**\brief Start processing
   *
   * Starts processing the incoming request.
   */
  void start(const std::string& upload_folder_path, int upload_file_id) {
    uploadedFilePath = upload_folder_path;
    if (upload_folder_path[upload_folder_path.length() - 1] != '/')
      uploadedFilePath.push_back('/');
    uploadedFilePath += std::to_string(upload_file_id);
    
    read();
  }

  /**\brief Send reply
   *
   * Used by the processing code once it is known how to answer the request
   * contained in this object. The code will automatically add a correct
   * Content-Length header, but further headers may be added using the free-form
   * header parameter. Headers are not checked for validity.
   *
   * \note The code will always reply with an HTTP/1.1 reply, regardless of the
   *       version in the request. If this is a concern for you, put the server
   *       behind an an nginx instance, which should fix up the output as
   *       necessary.
   *
   * \param[in] status The status to return.
   * \param[in] header Any additional headers to be sent.
   * \param[in] body   The response body to send back to the client.
   */
  void reply(int status, const std::string &header, const std::string &body) {
    
    std::shared_ptr<std::string> reply = std::shared_ptr<std::string>(new std::string());
    reply->reserve(header.length() + body.length() + 64);
    
    *reply += "HTTP/1.1 ";
    *reply += std::to_string(status);
    *reply += " NA\r\nContent-Length: ";
    *reply += std::to_string(body.length());
    *reply += "\r\n";

    /* we automatically close connections when an error code is sent. */
    if (status >= 400) {
      *reply += "Connection: close\r\n";
    }
    
    *reply += header;
    *reply += "\r\n";
    *reply += body;
    
    boost::asio::async_write(
                      socket, 
                      boost::asio::buffer(*reply),
                      [&, reply](const boost::system::error_code ec, std::size_t length) {
                        handleWrite(status, ec);
                      });

    //TODO: this doesn't work with UNIX sockets, so is disabled for now. I'd
    // probably have to overload something here, but not sure if it would be
    // worth it.
    //server.log << socket.remote_endpoint().address().to_string()
/*
    server.log << "-"
               << " - - [-] \"" << method << " " << resource
               << " HTTP/1.[01]\" " << status << " " << body.length()
               << " \"-\" \"-\"\n";
               */
  }

  /**\brief Send reply without custom headers
   *
   * Wraps around the 3-parameter reply() function, so that you don't have to
   * specify an empty header parameter if you don't intend to set custom
   * headers.
   *
   * \param[in] status The status to return.
   * \param[in] body   The response body to send back to the client.
   */
  void reply(int status, const std::string &body) { reply(status, "", body); }
  
  const std::string& get_uploaded_path() const { return uploadedFilePath; }

protected:
  /**\brief Read more data
   *
   * Called by ASIO to indicate that new data has been read and can now be
   * processed.
   *
   * The actual processing for the header is done with a set of regexen, which
   * greatly simplifies the header parsing.
   *
   * \param[in] error             Current error state.
   * \param[in] bytes_transferred The amount of data that has been read.
   */
  void handleRead(const boost::system::error_code &error, size_t bytes_transferred) {
    if (status == stShutdown) {
      return;
    }

    if (!error) {
      static const REGEX_NAMESPACE::regex req(
          "(\\w+)\\s+([\\w\\d%/.:;()+-?=&]+)\\s+HTTP/1.[01]\\s*");
      static const REGEX_NAMESPACE::regex mime("([\\w-]+):\\s*(.*)\\s*");
      static const REGEX_NAMESPACE::regex mimeContinued("[ \t]\\s*(.*)\\s*");

      std::istream is(input);
      std::string s;

      REGEX_NAMESPACE::smatch matches;

      switch (status) {
      case stRequest:
      case stHeader:
        std::getline(is, s);
        break;
      case stContent:
          if (bytes_transferred > 0) {
            totalBytesTransferred += bytes_transferred;
            
            const int buflen = 64 * 1024;
            FILE *fp = fopen(uploadedFilePath.c_str(), "ab");
            s = std::string(buflen, 0);
            if (fp) {
              for(int r = bytes_transferred; r > 0; r -= buflen) {
                int rlen = std::min<int>(r, buflen);
                is.read(&s[0], rlen);
                fwrite(&s[0], 1, rlen, fp);
              }
              fclose(fp);
            }
            if (totalBytesTransferred < contentLength) {
              read();
              return;
            }
          }
        break;
      case stProcessing:
      case stErrorContentTooLarge:
      case stShutdown:
        break;
      }

      switch (status) {
      case stRequest:
        if (REGEX_NAMESPACE::regex_match(s, matches, req)) {
          method = matches[1];
          resource = matches[2];

          header = std::map<std::string, std::string, caseInsensitiveLT>();
          status = stHeader;
        }
        break;

      case stHeader:
        if ((s == "\r") || (s == "")) {
          const auto &cli = header.find("Content-Length");

          if (cli != header.end()) {
            try {
              totalBytesTransferred = 0;
              contentLength = std::atoi(std::string(cli->second).c_str());
            }
            catch (...) {
              contentLength = 0;
            }

            if (contentLength > maxContentLength) {
              status = stErrorContentTooLarge;
              reply(400, "Request body too large");
            } else {
              status = stContent;
            }
            break;
          } else {
            status = stContent;
            s = "";
          }
        } else if (REGEX_NAMESPACE::regex_match(s, matches, mimeContinued)) {
          header[lastHeader] += "," + std::string(matches[1]);
          break;
        } else if (REGEX_NAMESPACE::regex_match(s, matches, mime)) {
          lastHeader = matches[1];
          header[matches[1]] = matches[2];
          break;
        }

      case stContent:
        content = s;
        status = stProcessing;

        /* processing the request takes places here */
        the_server.processor(*this, header);
          
        break;

      case stProcessing:
      case stErrorContentTooLarge:
      case stShutdown:
        break;
      }

      switch (status) {
      case stRequest:
      case stHeader:
      case stContent:
        read();
      case stProcessing:
      case stErrorContentTooLarge:
      case stShutdown:
        break;
      }
    } else {
      self.reset();
    }
  }

  /**\brief Asynchronouse write handler
   *
   * Decides whether or not things need to be written to the stream, or if
   * things need to be read instead.
   *
   * Automatically deletes the object on errors - which also closes the
   * connection automagically.
   *
   * \param[in] statusCode The HTTP status code of the reply.
   * \param[in] error      Current error state.
   */
  void handleWrite(int statusCode, const boost::system::error_code &error) {
    if (status == stShutdown) {
      return;
    }
    
    if (!error && (statusCode < 400)) {
      if (status == stProcessing) {
        status = stRequest;
        read();
      }
    } else {
      self.reset();
    }
  }

  /**\brief Asynchronouse read handler
   *
   * Decides if things need to be read in and - if so - what needs to be read.
   *
   * Automatically deletes the object on errors - which also closes the
   * connection automagically.
   */
  void read(void) {
    switch (status) {
    case stRequest:
    case stHeader:
      boost::asio::async_read_until(
                    socket, *input, "\n",
                    [&](const boost::system::error_code &error, std::size_t bytes_transferred){
                      handleRead(error, bytes_transferred);
                    }
                  );
      break;
    case stContent:
        boost::asio::async_read_until(
                               socket, *input, "\n",
                               [&](const boost::system::error_code & error, std::size_t bytes_transferred) {
                                 handleRead(error, bytes_transferred);
                               }
                              );
        break;
      case stProcessing:
    case stErrorContentTooLarge:
    case stShutdown:
      break;
    }
  }

  /**\brief Name of the last parsed header
   *
   * This is the name of the last header line that was parsed. Used with
   * multi-line headers.
   */
  std::string lastHeader;

  /**\brief Content length
   *
   * This is the value of the Content-Length header. Used when parsing a request
   * with a body.
   */
  std::size_t contentLength;
  
  /**\brief boost::asio input stream buffer
   *
   * This is the stream buffer that the object is reading from.
   */
  boost::asio::streambuf* input;
  
  /* added by Geolux for file upload support */
  std::string uploadedFilePath;
  
  int totalBytesTransferred;
  
  
};
}
}
}

#endif