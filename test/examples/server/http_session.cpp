//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "http_session.hpp"
#include <boost/config.hpp>
#include <iostream>
#include "websocket_session.hpp"

/// \brief This function produces an HTTP response for the given request
template <class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>> &&req,
                    Send &&send) {
  // Returns a bad request response
  auto const bad_http_request = [&req](beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not acceptable response
  auto const wrong_jrpc_request = [&req](beast::string_view target,
                                         std::string_view message) {
    http::response<http::string_body> res{http::status::not_acceptable,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = message;
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error = [&req](beast::string_view what,
                                   std::string_view message) {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = message;
    res.prepare_payload();
    return res;
  };

  std::cout << "===========request text===========\n" << req << std::endl;

  // process only GET and POST methods;
  if (req.method() != http::verb::get && req.method() != http::verb::post) {
    return send(bad_http_request("Unsupported HTTP-method"));
  }

  // TODO: implement handle request here

  std::cout << "request method = " << req.method() << std::endl;
  std::cout << "request target = " << req.target() << std::endl;
  std::cout << "request body = " << req.body() << std::endl;

  http::string_body::value_type body;
  body.assign(
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})");

  const auto size = body.size();

  // Respond to GET request
  http::response<http::string_body> res{
      std::piecewise_construct,
      std::make_tuple(std::move(body)),
      std::make_tuple(http::status::ok, req.version())};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html");
  res.content_length(size);
  res.keep_alive(req.keep_alive());
  return send(std::move(res));
}

HttpSession::HttpSession(tcp::socket socket) : stream_(std::move(socket)) {}

void HttpSession::start() {
  doRead();
}

// Report a failure
void HttpSession::fail(beast::error_code ec, char const *what) {
  // Don't report on canceled operations
  if (ec == net::error::operation_aborted) {
    return;
  }

  std::cerr << what << ": " << ec.message() << "\n";
}

void HttpSession::doRead() {
  // Construct a new parser for each message
  parser_.emplace();

  // Apply a reasonable limit to the allowed size
  // of the body in bytes to prevent abuse.
  parser_->body_limit(10000);

  // Set the timeout.
  stream_.expires_after(std::chrono::seconds(30));

  // Read a request
  http::async_read(stream_,
                   buffer_,
                   parser_->get(),
                   [self = shared_from_this()](auto ec, auto count) {
                     self->onRead(ec, count);
                   });
}

namespace {
  template <class T, typename D = std::decay_t<T>>
  std::shared_ptr<D> make_shared(T &&any) {
    return std::make_shared<D>(std::forward<T>(any));
  }
}  // namespace

void HttpSession::onRead(beast::error_code ec, std::size_t) {
  // This means they closed the connection
  if (ec == http::error::end_of_stream) {
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    return;
  }

  // Handle the error, if any
  if (ec) {
    return fail(ec, "read");
  }

  // See if it is a WebSocket Upgrade
  if (websocket::is_upgrade(parser_->get())) {
    // Create a websocket session, transferring ownership
    // of both the socket and the HTTP request.
    std::make_shared<websocket_session>(stream_.release_socket())
        ->run(parser_->release());
    return;
  }

  // Send the response
  handle_request(parser_->release(), [this](auto &&response) {
    using response_type = decltype(response);
    auto r = std::make_shared<std::decay_t<response_type>>(
        std::forward<response_type>(response));

    // Write the response
    http::async_write(
        stream_, *r, [self = shared_from_this(), r](auto ec, auto bytes) {
          self->onWrite(ec, bytes, r->need_eof());
        });
  });
}

void HttpSession::onWrite(beast::error_code ec, std::size_t, bool close) {
  // Handle the error, if any
  if (ec) {
    return fail(ec, "write");
  }

  if (close) {
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    return;
  }

  // Read another request
  doRead();
}
