//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "beast_listener.hpp"
#include <iostream>
#include "http_session.hpp"

BeastListener::BeastListener(net::io_context &ioc,
                             const tcp::endpoint &endpoint)
    : ioc_(ioc), acceptor_(ioc) {
  beast::error_code ec;

  // Open the acceptor
  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    fail(ec, "open");
    return;
  }

  // Allow address reuse
  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    fail(ec, "set_option");
    return;
  }

  // Bind to the server address
  acceptor_.bind(endpoint, ec);
  if (ec) {
    fail(ec, "bind");
    return;
  }

  // Start listening for connections
  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    fail(ec, "listen");
    return;
  }
}

void BeastListener::start() {
  acceptOnce();
}

// Report a failure
void BeastListener::fail(beast::error_code ec, char const *what) {
  // Don't report on canceled operations
  if (ec == net::error::operation_aborted) return;
  std::cerr << what << ": " << ec.message() << "\n";
}

void BeastListener::acceptOnce() {  // The new connection gets its own strand
  acceptor_.async_accept(net::make_strand(ioc_),
                         [self = shared_from_this()](auto &&ec, auto &&socket) {
                           using socket_type = std::decay_t<decltype(socket)>;
                           self->onAccept(ec,
                                          std::forward<socket_type>(socket));
                         });
}

// Handle a connection
void BeastListener::onAccept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    return fail(ec, "accept");
  }
  // Launch a new session for this connection
  boost::make_shared<HttpSession>(std::move(socket))->start();

  acceptOnce();
}
