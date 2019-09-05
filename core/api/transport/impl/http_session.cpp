/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_session.hpp"

#include <boost/config.hpp>
#include <iostream>

namespace kagome::api {

  /**
   * @brief process http request, compose and execute response
   * @tparam Body request body type
   * @tparam Allocator allocator type
   * @param req request
   * @param send sender function
   */
  template <class Body>
  void HttpSession::handleRequest(boost::beast::http::request<Body> &&req) {
    // Returns a bad request response
    auto const bad_http_request = [&req](std::string_view message) {
      boost::beast::http::response<boost::beast::http::string_body> res{
          boost::beast::http::status::bad_request, req.version()};
      res.set(boost::beast::http::field::server, kServerName);
      res.set(boost::beast::http::field::content_type, "text/html");
      res.keep_alive(req.keep_alive());
      res.body() = message;
      res.prepare_payload();
      return res;
    };

    // process only POST method
    if (req.method() != boost::beast::http::verb::post) {
      return asyncWrite(bad_http_request("Unsupported HTTP-method"));
    }

    onRequest()(req.body());
  }

  HttpSession::HttpSession(boost::asio::ip::tcp::socket socket,
                           HttpSession::Configuration config)
      : config_{config}, stream_(std::move(socket)) {
    onError().connect([](auto ec, auto &&message) {
      std::cerr << "http session error " << ec << ": " << message << std::endl;
    });

    onResponse().connect(
        [this](std::string_view message) { sendResponse(message); });
  }

  void HttpSession::start() {
    acyncRead();
  }

  void HttpSession::stop() {
    boost::system::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    boost::ignore_unused(ec);
  }

  void HttpSession::acyncRead() {
    parser_.emplace();
    parser_->body_limit(config_.max_request_size);
    stream_.expires_after(config_.operation_timeout);

    boost::beast::http::async_read(
        stream_,
        buffer_,
        parser_->get(),
        [self = shared_from_this()](auto ec, auto count) {
          self->onRead(ec, count);
        });
  }

  template <class Message>
  void HttpSession::asyncWrite(Message &&message) {
    // we need to put message into a shared ptr for async operations
    using message_type = decltype(message);
    auto m = std::make_shared<std::decay_t<message_type>>(
        std::forward<message_type>(message));

    // write response
    boost::beast::http::async_write(
        stream_, *m, [self = shared_from_this(), m](auto ec, auto size) {
          self->onWrite(ec, size, m->need_eof());
        });
  }

  void HttpSession::sendResponse(std::string_view response) {
    boost::beast::http::string_body::value_type body;
    body.assign(response);

    const auto size = body.size();

    // send response
    boost::beast::http::response<boost::beast::http::string_body> res(
        std::piecewise_construct, std::make_tuple(std::move(body)));
    res.set(boost::beast::http::field::server, kServerName);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.content_length(size);
    res.keep_alive(true);

    return asyncWrite(std::move(res));
  }

  void HttpSession::onRead(boost::system::error_code ec, std::size_t) {
    if (ec == boost::beast::http::error::end_of_stream) {
      stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                                ec);
      return onError()(ec, "connection was closed");
    }

    if (ec) {
      onError()(ec, "error occurred");
      return stop();
    }

    handleRequest(parser_->release());
  }

  void HttpSession::onWrite(boost::system::error_code ec,
                            std::size_t,
                            bool should_stop) {
    if (ec) {
      return onError()(ec, "failed to write message");
    }

    if (should_stop) {
      return stop();
    }

    // read next request
    acyncRead();
  }
}  // namespace kagome::api
