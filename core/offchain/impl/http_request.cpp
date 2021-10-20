/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/http_request.hpp"

#include <algorithm>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace kagome::offchain {

  HttpRequest::HttpRequest(boost::asio::io_context &io_context, RequestId id)
      : io_context_(io_context),
        id_(id),
        resolver_(io_context_),
        ssl_ctx_(boost::asio::ssl::context::sslv23),
        deadline_timer_(io_context_),
        log_(log::createLogger("HttpRequest#" + std::to_string(id_),
                               "offchain")) {
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_ctx_.set_verify_callback(
        [log = log_](bool preverified, boost::asio::ssl::verify_context &ctx) {
          // The verify callback can be used to check whether the certificate
          // that is being presented is valid for the peer. For example, RFC
          // 2818 describes the steps involved in doing this for HTTPS. Consult
          // the OpenSSL documentation for more details. Note that the callback
          // is called once for each certificate in the certificate chain,
          // starting from the root certificate authority
          //
          // In this example we will simply print the certificate's subject name
          char subject_name[256];
          X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
          SL_TRACE(log, "Verifying {}", subject_name);

          return preverified;
        });
  }

  bool HttpRequest::init(Method method,
                         std::string_view uri_arg,
                         common::Buffer meta) {
    uri_ = Uri::Parse(uri_arg);
    if (uri_.Schema != "https" and uri_.Schema != "http") {
      SL_ERROR(log_, "URI has invalid schema: `{}`", uri_.Schema);
      return false;
    }
    if (not uri_.Port.empty()) {
      if (int port = std::stoi(uri_.Port);
          port <= 0 or port > 65536 or uri_.Port != std::to_string(port)) {
        SL_ERROR(log_, "URI has invalid port: `{}`", uri_.Port);
        return false;
      }
    } else if (uri_.Schema == "https") {
      uri_.Port = "443";
    } else if (uri_.Schema == "http") {
      uri_.Port = "80";
    }
    if (uri_.Host.empty()) {
      SL_ERROR(log_, "URI has empty host");
      return false;
    }
    if (uri_.Path.empty()) {
      SL_ERROR(log_, "URI has empty path");
      return false;
    }

    if (uri_.Schema == "https") {
      secure_ = true;
      stream_ = std::make_unique<SslStream>(io_context_, ssl_ctx_);
    } else if (uri_.Schema == "http") {
      secure_ = false;
      stream_ = std::make_unique<TcpStream>(io_context_);
    }

    SL_DEBUG(log_, "Initialized for URL: {}", uri_.toString());

    if (method == Method::POST) {
      request_.method(boost::beast::http::verb::post);
    } else if (method == Method::GET) {
      request_.method(boost::beast::http::verb::get);
    }
    request_.target(uri_.Path);
    request_.version(11);  // HTTP/1.1
    request_.set(boost::beast::http::field::host, uri_.Host);
    request_.set(boost::beast::http::field::user_agent, "KagomeOffchainWorker");
    request_.set(boost::beast::http::field::connection, "Close");

    resolve();
    return true;
  }

  void HttpRequest::resolve() {
    if (status_ != 0) {
      return;
    }

    SL_TRACE(log_, "Resolve hostname {}", uri_.Host);

    if (secure_) {
      auto &stream = *boost::relaxed_get<SslStreamPtr>(stream_);

      // Set SNI Hostname (many hosts need this to handshake successfully)
      if (!SSL_set_tlsext_host_name(stream.native_handle(),
                                    uri_.Host.c_str())) {
        boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                    boost::asio::error::get_ssl_category()};
        SL_ERROR(
            log_, "Can't resolve hostname {}: {}", uri_.Host, ec.message());
        status_ = HttpStatus(20);
        return;
      }
    }

    auto resolve_handler = [wp = weak_from_this()](const auto &ec, auto it) {
      if (auto self = wp.lock()) {
        std::cerr << "RESOLVE HANDLER" << std::endl;
        if (self->status_ != 0) {
          SL_TRACE(
              self->log_, "Result of resilving is ignored: {}", self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Resolved hostname {}", self->uri_.Host);
          self->resolver_iterator_ = it;
          self->connect();
          return;
        }

        std::cerr << "Can't resolve hostname: " << ec.message() << std::endl;

        SL_ERROR(self->log_,
                 "Can't resolve hostname {}: {}",
                 self->uri_.Host,
                 ec.message());
        self->status_ = HttpStatus(20);
      }
    };

    resolver_.async_resolve(uri_.Host, uri_.Port, std::move(resolve_handler));
  }

  void HttpRequest::connect() {
    if (status_ != 0) {
      return;
    }

    SL_TRACE(log_,
             "Connect to `{}` (addr={}:{})",
             uri_.Host,
             resolver_iterator_->endpoint().address().to_string(),
             resolver_iterator_->endpoint().port());

    auto &stream = secure_ ? boost::beast::get_lowest_layer(
                       *boost::relaxed_get<SslStreamPtr>(stream_))
                           : boost::beast::get_lowest_layer(
                               *boost::relaxed_get<TcpStreamPtr>(stream_));

    auto connect_handler = [wp = weak_from_this()](const auto &ec, auto it) {
      if (auto self = wp.lock()) {
        std::cerr << "CONNECT HANDLER" << std::endl;
        if (self->status_ != 0) {
          SL_TRACE(
              self->log_, "Result of connecting is ignored: {}", self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Connection established");
          if (self->secure_) {
            self->handshake();
          } else {
            self->connected_ = true;
            self->sendRequest();
          }
          return;
        }

        std::cerr << "Connection failed: " << ec.message() << std::endl;

        SL_ERROR(self->log_, "Connection failed: {}", ec.message());

        // Try to connect next endpoint if any
        if (++self->resolver_iterator_
            != boost::asio::ip::tcp::resolver::iterator{}) {
          SL_TRACE(self->log_, "Trying next endpoint...");
          self->connect();
        } else {
          self->status_ = HttpStatus(20);
        }
      }
    };

    stream.async_connect(resolver_iterator_, {}, std::move(connect_handler));
  }

  void HttpRequest::handshake() {
    if (status_ != 0) {
      return;
    }

    BOOST_ASSERT(secure_);

    auto &stream = *boost::relaxed_get<SslStreamPtr>(stream_);

    auto handshake_handler = [wp = weak_from_this()](const auto &ec) {
      if (auto self = wp.lock()) {
        std::cerr << "HANDSHAKE HANDLER" << std::endl;
        if (self->status_ != 0) {
          SL_TRACE(
              self->log_, "Result of handshake is ignored: {}", self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Handshake successful");
          self->connected_ = true;
          self->sendRequest();
          return;
        }

        std::cerr << "Handshake failed: " << ec.message() << std::endl;

        SL_ERROR(self->log_, "Handshake failed: {}", ec.message());
        self->status_ = HttpStatus(20);
      }
    };

    stream.async_handshake(boost::asio::ssl::stream_base::client,
                           std::move(handshake_handler));
  }

  void HttpRequest::sendRequest() {
    if (status_ != 0) {
      return;
    }

    if (not request_is_ready_) {
      SL_TRACE(log_, "Request not ready (body is not finalised)");
      return;
    }
    if (not connected_) {
      SL_TRACE(log_, "Request not ready (connection is not established)");
      return;
    }

    if (request_.method() == boost::beast::http::verb::post) {
      request_.set(boost::beast::http::field::content_length,
                   std::to_string(request_.body().size()));
    }

    std::cerr << "Method> " << request_.method_string() << "\n";
    std::cerr << "Path> " << request_.target() << "\n";
    std::cerr << "Version> " << request_.version() << "\n";

    for (auto it = request_.cbegin(); it != request_.cend(); ++it) {
      std::cerr << "Header> " << it->name_string() << ": " << it->value()
                << "\n";
    }

    if (request_.method() == boost::beast::http::verb::post) {
      std::cerr << "BeginBody>\n";
      std::cerr.write(static_cast<char *>(request_.body().data()),
                      request_.body().size());
      std::cerr << "\nEndBody>\n" << std::endl;
    } else {
      std::cerr << std::endl;
    }

    auto serializer = std::make_shared<boost::beast::http::request_serializer<
        boost::beast::http::string_body>>(request_);

    auto write_handler = [wp = weak_from_this(), serializer](const auto &ec,
                                                             auto written) {
      if (auto self = wp.lock()) {
        std::cerr << "WRITE HANDLER" << std::endl;
        self->resetDeadline();
        if (self->status_ != 0) {
          SL_TRACE(self->log_,
                   "Result of request sending is ignored: {}",
                   self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Request has sent successful");
          self->recvResponse();
          return;
        }

        std::cerr << "Request failed: " << ec.message() << std::endl;

        SL_ERROR(self->log_, "Request send was fail: {}", ec.message());
        self->status_ = HttpStatus(20);
      }
    };

    // Send the HTTP request to the remote host
    if (secure_) {
      auto &stream = *boost::relaxed_get<SslStreamPtr>(stream_);
      boost::beast::http::async_write(
          stream, *serializer, std::move(write_handler));
    } else {
      auto &stream = *boost::relaxed_get<TcpStreamPtr>(stream_);
      boost::beast::http::async_write(
          stream, *serializer, std::move(write_handler));
    }
  }

  void HttpRequest::recvResponse() {
    if (status_ != 0) {
      return;
    }

    SL_TRACE(log_, "Read response");

    auto read_handler = [wp = weak_from_this()](const auto &ec, auto received) {
      if (auto self = wp.lock()) {
        std::cerr << "READ HANDLER" << std::endl;
        self->resetDeadline();
        if (self->status_ != 0) {
          SL_TRACE(self->log_,
                   "Result of response receiving is ignored: {}",
                   self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Response has received successful", received);
          self->done();
          return;
        }

        std::cerr << "Response receive was fail: " << ec.message() << std::endl;

        SL_ERROR(self->log_, "Response receive was fail: {}", ec.message());
        self->status_ = HttpStatus(20);
      }
    };

    if (secure_) {
      auto &stream = *boost::relaxed_get<SslStreamPtr>(stream_);
      boost::system::error_code ec;
      boost::beast::get_lowest_layer(stream).socket().shutdown(
          boost::asio::ip::tcp::socket::shutdown_send, ec);
      boost::beast::http::async_read(
          stream, buffer_, parser_, std::move(read_handler));
    } else {
      auto &stream = *boost::relaxed_get<TcpStreamPtr>(stream_);
      boost::system::error_code ec;
      boost::beast::get_lowest_layer(stream).socket().shutdown(
          boost::asio::ip::tcp::socket::shutdown_send, ec);
      boost::beast::http::async_read(
          stream, buffer_, parser_, std::move(read_handler));
    }
  }

  void HttpRequest::done() {
    if (status_ != 0) {
      return;
    }

    if (secure_) {
      auto &stream = *boost::relaxed_get<SslStreamPtr>(stream_);
      boost::system::error_code ec;
      boost::beast::get_lowest_layer(stream).socket().shutdown(
          boost::asio::ip::tcp::socket::shutdown_both, ec);
    } else {
      auto &stream = *boost::relaxed_get<TcpStreamPtr>(stream_);
      boost::system::error_code ec;
      boost::beast::get_lowest_layer(stream).socket().shutdown(
          boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    response_ = parser_.release();

    status_ = response_.result_int();

    std::cerr << "Status> " << response_.result_int() << " "
              << obsolete_reason(response_.result()).to_string() << "\n";

    for (auto it = response_.cbegin(); it != response_.cend(); ++it) {
      std::cerr << "Header> " << it->name_string() << ": " << it->value()
                << "\n";
    }

    std::cerr << "BeginBody>\n";
    std::cerr.write(static_cast<char *>(response_.body().data()),
                    response_.body().size());
    std::cerr << "\nEndBody>\n" << std::endl;
  }

  void HttpRequest::setDeadline(std::chrono::milliseconds delay) {
    if (status_ != 0) {
      return;
    }
    std::cerr << "SET DEADLINE FOR " << delay.count() << std::endl;

    boost::system::error_code ec;
    deadline_timer_.cancel(ec);
    deadline_timer_.async_wait([wp = weak_from_this()](const auto &ec) {
      if (auto self = wp.lock()) {
        if (!ec) {
          self->status_ = HttpStatus(10);  // Deadline was reached
        }
      }
    });
    deadline_timer_.expires_from_now(delay);
  }

  void HttpRequest::resetDeadline() {
    std::cerr << "RESET DEADLINE" << std::endl;
    boost::system::error_code ec;
    deadline_timer_.cancel(ec);
  }

  RequestId HttpRequest::id() const {
    return id_;
  }

  HttpStatus HttpRequest::status() const {
    return status_;
  }

  Result<Success, Failure> HttpRequest::addRequestHeader(
      std::string_view name, std::string_view value) {
    if (not adding_headers_is_allowed_) {
      SL_ERROR(log_, "Trying to add header into ready request");
      return Failure();
    }

    request_.insert(boost::string_view(name.begin(), name.size()), value);

    return Success();
  }

  Result<Success, HttpError> HttpRequest::writeRequestBody(
      const common::Buffer &chunk,
      boost::optional<std::chrono::milliseconds> deadline) {
    if (request_has_sent_) {
      SL_ERROR(log_, "Trying to write body into ready request");
      return HttpError::IoError;
    }

    if (adding_headers_is_allowed_) {
      adding_headers_is_allowed_ = false;
    }

    if (chunk.empty()) {
      if (deadline.has_value()) {
        setDeadline(deadline.value());
      }
      request_has_sent_ = true;
      request_is_ready_ = true;
      sendRequest();
    } else {
      request_.body().append(reinterpret_cast<const char *>(chunk.data()),
                             chunk.size());
    }

    return Result<Success, HttpError>();
  }

  std::vector<std::pair<std::string, std::string>>
  HttpRequest::getResponseHeaders() const {
    std::vector<std::pair<std::string, std::string>> result;
    for (auto &header : response_) {
      result.emplace_back(std::pair(header.name_string(), header.value()));
    }
    return result;
  }

  Result<uint32_t, HttpError> HttpRequest::readResponseBody(
      common::Buffer &chunk,
      boost::optional<std::chrono::milliseconds> deadline) {
    switch (status_) {
      case 0:
        return HttpError::InvalidId;
      case 10:
        return HttpError::Timeout;
      case 20:
        return HttpError::IoError;
    }

    auto amount = std::min(response_.body().size(), chunk.size());

    std::copy_n(response_.body().begin(), amount, chunk.begin());

    return amount;
  }
}  // namespace kagome::offchain
