//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

//------------------------------------------------------------------------------
/*
    WebSocket chat server, multi-threaded

    This implements a multi-user chat room using WebSocket. The
    `io_context` runs on any number of threads, specified at
    the command line.

*/
//------------------------------------------------------------------------------

#include "beast_listener.hpp"

#include <boost/asio/signal_set.hpp>
#include <boost/smart_ptr.hpp>
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
  auto address = net::ip::make_address("0.0.0.0");
  constexpr size_t port = 8080;
  constexpr size_t threads = 1;

  // The io_context is required for all I/O
  net::io_context ioc;

  // Create and launch a listening port
  boost::make_shared<BeastListener>(ioc, tcp::endpoint{address, port})->start();

  // Capture SIGINT and SIGTERM to perform a clean shutdown
  net::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&ioc](boost::system::error_code const &, int) {
    // Stop the io_context. This will cause run()
    // to return immediately, eventually destroying the
    // io_context and any remaining handlers in it.
    ioc.stop();
  });

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i) {
    v.emplace_back([&ioc] { ioc.run(); });
  }
  ioc.run();

  // (If we get here, it means we got a SIGINT or SIGTERM)

  // Block until all the threads exit
  for (auto &t : v) {
    t.join();
  }

  return 0;
}
