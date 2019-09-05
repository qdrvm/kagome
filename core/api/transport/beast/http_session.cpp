/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/beast/http_session.hpp"

#include <boost/config.hpp>
#include <iostream>

namespace kagome::api {

  /**
   * @brief process http request, compose and execute response
   * @tparam Body request body type
   * @tparam Allocator allocator type
   * @tparam Send sender lambda
   * @param req request
   * @param send sender function
   */
  template <class Body, class Allocator, class Send>
  void HttpSession::handle_request(
      boost::beast::http::request<Body,
                                  boost::beast::http::basic_fields<Allocator>>
          &&req,
      Send &&send) {
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

    //    // Returns a server error response
    //    auto const server_error = [&req](std::string_view what,
    //                                     std::string_view message) {
    //      boost::beast::http::response<boost::beast::http::string_body> res{
    //          boost::beast::http::status::internal_server_error,
    //          req.version()};
    //      res.set(boost::beast::http::field::server,
    //      BOOST_BEAST_VERSION_STRING);
    //      res.set(boost::beast::http::field::content_type, "text/html");
    //      res.keep_alive(req.keep_alive());
    //      res.body() = message;
    //      res.prepare_payload();
    //      return res;
    //    };

    // process only POST method
    if (req.method() != boost::beast::http::verb::post) {
      return send(bad_http_request("Unsupported HTTP-method"));
    }

    auto &&request_message = req.body();
    onRequest()(request_message);

    // TODO: implement handle request here

    boost::beast::http::string_body::value_type body;
    //    body.assign(
    //        R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})");

    const auto size = body.size();

    // Respond to GET request
    boost::beast::http::response<boost::beast::http::string_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, req.version())};
    res.set(boost::beast::http::field::server, kServerName);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
  }

  HttpSession::HttpSession(boost::asio::ip::tcp::socket socket,
                           HttpSession::Configuration config)
      : config_{config}, stream_(std::move(socket)) {
        onError().connect([](auto ec, auto &&message) {
          std::cerr << "http session error " << ec << ": " << message << std::endl;
        });
      }

  void HttpSession::start() {
    doRead();
  }

  void HttpSession::stop() {
    boost::system::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    stream_.cancel();
    boost::ignore_unused(ec);
  }

  void HttpSession::doRead() {
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

  void HttpSession::onRead(boost::system::error_code ec, std::size_t) {
    // This means they closed the connection
    if (ec == boost::beast::http::error::end_of_stream) {
      stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                                ec);
      return;
    }

    if (ec) {

      return stop();
    }

    // Send the response
    handle_request(parser_->release(), [this](auto &&response) {
      using response_type = decltype(response);
      auto r = std::make_shared<std::decay_t<response_type>>(
          std::forward<response_type>(response));

      // Write the response
      boost::beast::http::async_write(
          stream_, *r, [self = shared_from_this(), r](auto ec, auto bytes) {
            self->onWrite(ec, bytes, r->need_eof());
          });
    });
  }

  void HttpSession::onWrite(boost::system::error_code ec,
                            std::size_t,
                            bool close) {
    // Handle the error, if any
    if (ec) {
      return onError()(ec, "failed to write message");
    }

    if (close) {
      return stop();
    }

    // Read another request
    doRead();
  }

}  // namespace kagome::api
