//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "websocket_session.hpp"
#include <iostream>

websocket_session::websocket_session(tcp::socket socket)
    : ws_(std::move(socket)) {}

void websocket_session::fail(beast::error_code ec, char const *what) {
  // Don't report these
  if (ec == net::error::operation_aborted || ec == websocket::error::closed) {
    return;
  }

  std::cerr << what << ": " << ec.message() << "\n";
}

void websocket_session::on_accept(beast::error_code ec) {
  // Handle the error, if any
  if (ec) {
    return fail(ec, "accept");
  }

  // Read a message
  ws_.async_read(buffer_, [self = shared_from_this()](auto ec, auto size) {
    self->on_read(ec, size);
  });
}

void websocket_session::on_read(beast::error_code ec, std::size_t) {
  // Handle the error, if any
  if (ec) {
    return fail(ec, "read");
  }

  // Clear the buffer
  buffer_.consume(buffer_.size());

  // Read another message
  ws_.async_read(buffer_, [self = shared_from_this()](auto ec, auto size) {
    self->on_read(ec, size);
  });
}

void websocket_session::send(boost::shared_ptr<std::string const> const &ss) {
  // Post our work to the strand, this ensures
  // that the members of `this` will not be
  // accessed concurrently.

  net::post(ws_.get_executor(),
            [self = shared_from_this(), s = ss]() { self->on_send(s); });
}

void websocket_session::on_send(
    boost::shared_ptr<std::string const> const &ss) {
  // Always add to queue
  queue_.push_back(ss);

  // Are we already writing?
  if (queue_.size() > 1) {
    return;
  }

  // We are not currently writing, so send this immediately
  ws_.async_write(net::buffer(*queue_.front()),
                  [self = shared_from_this()](auto ec, auto size) {
                    self->on_write(ec, size);
                  });
}

void websocket_session::on_write(beast::error_code ec, std::size_t) {
  // Handle the error, if any
  if (ec) {
    return fail(ec, "write");
  }

  // Remove the string from the queue
  queue_.erase(queue_.begin());

  // Send the next message if any
  if (!queue_.empty()) {
    ws_.async_write(net::buffer(*queue_.front()),
                    [self = shared_from_this()](auto ec, auto size) {
                      self->on_write(ec, size);
                    });
  }
}
