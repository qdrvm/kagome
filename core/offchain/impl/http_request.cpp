/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/http_request.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http.hpp>

namespace kagome::offchain {

  HttpRequest::HttpRequest(RequestId id)
      : id_(id),
        resolver_(io_context_),
        ssl_ctx_(boost::asio::ssl::context::sslv23),
        deadline_timer_(io_context_),
        log_(log::createLogger("HttpRequest#" + std::to_string(id_),
                               "offchain")) {
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_ctx_.set_verify_callback(
        [log = log_, wp = weak_from_this()](
            bool preverified, boost::asio::ssl::verify_context &ctx) {
          // We will simply print the certificate's subject name here
          char subject_name[256];
          X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
          SL_WARN(log,
                  "Verifying [{}] was {}",
                  subject_name,
                  preverified ? "Successful" : "Failed");

          return preverified;
        });
  }

  bool HttpRequest::init(HttpMethod method,
                         std::string_view uri_arg,
                         common::Buffer meta) {
    uri_ = common::Uri::parse(uri_arg);
    if (uri_.error().has_value()) {
      error_message_ =
          ::fmt::format("URI parsing was failed: {}", uri_.error().value());
      SL_ERROR(log_, "{}", error_message_);
      return false;
    }
    if (uri_.Schema != "https" and uri_.Schema != "http") {
      error_message_ =
          ::fmt::format("URI has invalid schema: `{}`", uri_.Schema);
      SL_ERROR(log_, "{}", error_message_);
      return false;
    }
    if (not uri_.Port.empty()) {
      if (int port = std::stoi(std::string(uri_.Port));
          port <= 0 or port > 65536 or uri_.Port != std::to_string(port)) {
        error_message_ = ::fmt::format("URI has invalid port: `{}`", uri_.Port);
        SL_ERROR(log_, "{}", error_message_);
        return false;
      }
    } else if (uri_.Schema == "https") {
      uri_.Port = "443";
    } else if (uri_.Schema == "http") {
      uri_.Port = "80";
    }
    if (uri_.Host.empty()) {
      error_message_ = "URI has empty host";
      SL_ERROR(log_, "{}", error_message_);
      return false;
    }
    if (uri_.Path.empty()) {
      error_message_ = "URI has empty path";
      SL_ERROR(log_, "{}", error_message_);
      return false;
    }

    if (uri_.Schema == "https") {
      secure_ = true;
      stream_ = std::make_unique<SslStream>(io_context_, ssl_ctx_);
    } else if (uri_.Schema == "http") {
      secure_ = false;
      stream_ = std::make_unique<TcpStream>(io_context_);
    }

    SL_DEBUG(log_, "Initialized for URL: {}", uri_.to_string());

    if (method == HttpMethod::Post) {
      request_.method(boost::beast::http::verb::post);
    } else if (method == HttpMethod::Get) {
      request_.method(boost::beast::http::verb::get);
    }
    request_.target(std::string(uri_.Path));
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
                                    std::string(uri_.Host).c_str())) {
        boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                    boost::asio::error::get_ssl_category()};
        error_message_ =
            ::fmt::format("Can't resolve hostname {}: {}", uri_.Host, ec);
        SL_ERROR(log_, "{}", error_message_);
        status_ = ErrorHasOccurred;
        return;
      }
    }

    auto resolve_handler = [wp = weak_from_this()](const auto &ec, auto it) {
      if (auto self = wp.lock()) {
        if (self->status_ != 0) {
          SL_TRACE(
              self->log_, "Result of resolving is ignored: {}", self->status_);
          return;
        }

        if (!ec) {
          SL_TRACE(self->log_, "Resolved hostname {}", self->uri_.Host);
          self->resolver_iterator_ = it;
          self->connect();
          return;
        }

        self->error_message_ =
            ::fmt::format("Can't resolve hostname {}: {}", self->uri_.Host, ec);
        SL_ERROR(self->log_, "{}", self->error_message_);
        self->status_ = ErrorHasOccurred;
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

        SL_ERROR(self->log_, "Connection failed: {}", ec);

        // Try to connect next endpoint if any
        if (++self->resolver_iterator_
            != boost::asio::ip::tcp::resolver::iterator{}) {
          SL_TRACE(self->log_, "Trying next endpoint...");
          self->connect();
        } else {
          self->error_message_ = ::fmt::format("Connection failed: {}", ec);
          self->status_ = ErrorHasOccurred;
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

        self->error_message_ = ::fmt::format("Handshake failed: {}", ec);
        SL_ERROR(self->log_, "{}", self->error_message_);
        self->status_ = ErrorHasOccurred;
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
      SL_TRACE(log_, "Request not ready (body is not finalized)");
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

    auto serializer = std::make_shared<boost::beast::http::request_serializer<
        boost::beast::http::string_body>>(request_);

    auto write_handler = [wp = weak_from_this(), serializer](const auto &ec,
                                                             auto written) {
      if (auto self = wp.lock()) {
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

        self->error_message_ = ::fmt::format("Request send was fail: {}", ec);
        SL_ERROR(self->log_, "{}", self->error_message_);
        self->status_ = ErrorHasOccurred;
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

        self->error_message_ =
            ::fmt::format("Response reception has failed: {}", ec);
        SL_ERROR(self->log_, "{}", self->error_message_);
        self->status_ = ErrorHasOccurred;
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
      error_message_ = "Trying to add header into ready request";
      SL_ERROR(log_, "{}", error_message_);
      return Failure();
    }

    request_.insert(boost::string_view(name.begin(), name.size()), value);

    return Success();
  }

  Result<Success, HttpError> HttpRequest::writeRequestBody(
      const common::Buffer &chunk,
      std::optional<std::chrono::milliseconds> deadline_opt) {
    if (request_has_sent_) {
      error_message_ = "Trying to write body into ready request";
      SL_ERROR(log_, "{}", error_message_);
      return HttpError::IoError;
    }

    if (adding_headers_is_allowed_) {
      adding_headers_is_allowed_ = false;
    }

    if (chunk.empty()) {
      request_has_sent_ = true;
      request_is_ready_ = true;
      sendRequest();
      if (deadline_opt.has_value()) {
        auto &deadline = deadline_opt.value();
        io_context_.run_for(deadline);
        if (status_ == 0) {
          error_message_ = "Deadline has reached";
          status_ = DeadlineHasReached;
        }
      } else {
        io_context_.run();
      }

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
      std::optional<std::chrono::milliseconds> deadline) {
    switch (status_) {
      case InvalidIdentifier:
        error_message_ = "Invalid identifier";
        return HttpError::InvalidId;
      case DeadlineHasReached:
        error_message_ = "Deadline has reached";
        return HttpError::Timeout;
      case ErrorHasOccurred:
        error_message_ = "IO error happened";
        return HttpError::IoError;
    }

    auto amount = std::min(response_.body().size(), chunk.size());

    std::copy_n(response_.body().begin(), amount, chunk.begin());

    return amount;
  }
}  // namespace kagome::offchain
