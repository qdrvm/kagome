//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_LISTENER_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_LISTENER_HPP

#include <boost/smart_ptr.hpp>
#include <memory>
#include <string>
#include "beast.hpp"
#include "net.hpp"

// Accepts incoming connections and launches the sessions
class BeastListener : public boost::enable_shared_from_this<BeastListener> {
 public:
  BeastListener(net::io_context &ioc, const tcp::endpoint &endpoint);

  // Start accepting incoming connections
  void start();

 private:
  boost::asio::io_context &ioc_;
  boost::asio::ip::tcp::acceptor acceptor_;

  void fail(beast::error_code ec, char const *what);
  void acceptOnce();
  void onAccept(beast::error_code ec, tcp::socket socket);
};

#endif
