/**\file
 * \brief asio.hpp Generic Server
 *
 * A generic asynchronous server template using asio.hpp.
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

#if !defined(EF_GY_SERVER_H)
#define EF_GY_SERVER_H

#include <string>
#include <sstream>
#include <iostream>
#include <memory>

// #define ASIO_STANDALONE
#include <boost/asio.hpp>



namespace efgy {

static const unsigned int version = 8;


/**\brief Networking code
 *
 * Contains templates that deal with networking in one way or another. This code
 * is based on asio.hpp, an asynchronous I/O library for C++. The makefile knows
 * how to download this header-only library in case it's not installed.
 */
namespace net {
/**\brief Basic asynchronous server wrapper
 *
 * Contains the code that accepts incoming requests and dispatches sessions to
 * process these requests asynchronously.
 *
 * \tparam base             The socket class, e.g. asio::ip::tcp
 * \tparam requestProcessor The functor class to handle requests.
 */
template <typename base, typename requestProcessor,
          template <typename, typename> class sessionTemplate>
class server {
public:
  /**\brief Request session type
   *
   * Convenient typedef for a session as used by this server. Can come in handy
   * when writing functions for http::processor::base.
   */
  typedef sessionTemplate<base, requestProcessor> session;

  /**\brief Initialise with IO service
   *
   * Default constructor which binds an IO service to a socket endpoint that was
   * passed in. The socket is bound asynchronously.
   *
   * \param[out] pio IO service to use.
   * \param[in]  endpoint   Endpoint for the socket to bind.
   * \param[in]  upload_folder   The folder where uploaded files will be stored to
   * \param[out] logfile    A stream to write log messages to.
   */
  server(boost::asio::io_service &pio, typename base::endpoint &endpoint, const std::string& upload_folder,
         std::ostream &logfile = std::cout)
      : io(pio), acceptor(pio, endpoint), upload_folder(upload_folder), log(logfile), processor(), next_upload_file_id(0),
        name("server"), version("libefgy/") {
    std::ostringstream ver("");
    ver << efgy::version;
    version = version + ver.str();

    startAccept();
  }

  /**\brief Request processor instance
   *
   * Used to generate replies for incoming queries.
   */
  requestProcessor processor;

  /**\brief IO service
   *
   * Bound in the constructor to an asio.hpp IO service which handles
   * asynchronous connections.
   */
  boost::asio::io_service &io;

  /**\brief Log stream
   *
   * This is a standard output stream to send log data to. Log data is written
   * by the session code, so that code determines the format of log lines.
   */
  std::ostream &log;

  /**\brief Node name
   *
   * A name for the endpoint; some server protocols may need this. By default
   * this is populated with "server" in the constructor, so you have to set this
   * yourself if the kind of server you have needs this.
   */
  std::string name;

  /**\brief Software version
   *
   * A "software version" string, which is used by some server protocols.
   * Defaults to "libefgy/<version>".
   */
  std::string version;
  
  std::string upload_folder;
  
  int next_upload_file_id;

protected:
  /**\brief Accept the next incoming connection
   *
   * This function creates a new, blank session to handle the next incoming
   * request.
   */
  void startAccept(void) {
    std::shared_ptr<session> newSession = (new session(*this))->self;
    acceptor.async_accept(newSession->socket,
                          [newSession, this](const boost::system::error_code & error) {
      handleAccept(newSession, error);
    });
  }

  /**\brief Handle next incoming connection
   *
   * Called by asio.hpp when a new inbound connection has been accepted; this
   * will make the session parse the incoming request and dispatch it to the
   * request processor specified as a template argument.
   *
   * \param[in] newSession The blank session object that was created by
   *                       startAccept().
   * \param[in] error      Describes any error condition that may have occurred.
   */
  void handleAccept(std::shared_ptr<session> newSession,
                    const boost::system::error_code &error) {
    if (!error) {
      newSession->start(upload_folder, next_upload_file_id++);
    } else {
      newSession->self.reset();
    }
    startAccept();
  }

  /**\brief Socket acceptor
   *
   * This is the acceptor which has been bound to the socket specified in the
   * constructor.
   */
  typename base::acceptor acceptor;
};
}
}

#endif
