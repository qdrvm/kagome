/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/http_request.hpp"

#include <algorithm>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace kagome::offchain {

  HttpRequest::HttpRequest(boost::asio::io_context &io_context, RequestId id)
      : io_context_(io_context),
        id_(id),
        resolver_(io_context_),
        ssl_ctx_(boost::asio::ssl::context::sslv23),
        socket_(io_context_, ssl_ctx_),
        log_(log::createLogger("HttpRequest#" + std::to_string(id_),
                               "offchain")) {}

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
    } else if (uri_.Schema == "http") {
      secure_ = false;
    }

    SL_DEBUG(log_, "Initialized for URL: {}", uri_.toString());

    {  // Write necessary headers
      std::ostringstream oss;
      oss << (method_ == Method::GET ? "GET " : "POST ") << uri_.Path
          << (uri_.Query.empty() ? "" : "?") << uri_.Query << " HTTP/1.1\r\n"
          << "Host: " << uri_.Host << (uri_.Port.empty() ? "" : ":")
          << uri_.Port << "\r\n"
          << "Connection: close\r\n";
      auto headers = oss.str();

      boost::asio::buffer_copy(
          send_buffer_.prepare(headers.size()),
          boost::asio::const_buffer(headers.data(), headers.size()));
      send_buffer_.commit(headers.size());
    }

    doResolving();

    return true;
  }

  void HttpRequest::doResolving() {
    SL_TRACE(log_, "Resolving hostname `{}`", uri_.Host);

    resolver_.async_resolve(
        uri_.Host,
        uri_.Port,
        [wp = weak_from_this()](const boost::system::error_code &ec,
                                boost::asio::ip::tcp::resolver::iterator it) {
          if (auto self = wp.lock()) {
            if (!ec) {
              SL_TRACE(self->log_, "Resolved hostname `{}`", self->uri_.Host);
              self->resolver_iterator_ = it;
              self->onResolved();
              return;
            }

            SL_ERROR(self->log_,
                     "Can't resolve hostname `{}`: {}",
                     self->uri_.Host,
                     ec.message());
            self->status_ = HttpStatus(20);
          }
        });
  }

  void HttpRequest::onResolved() {
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(
        [log = log_](bool preverified, boost::asio::ssl::verify_context &ctx) {
          // The verify callback can be used to check whether the
          // certificate that is being presented is valid for the peer.
          // For example, RFC 2818 describes the steps involved in doing
          // this for HTTPS. Consult the OpenSSL documentation for more
          // details. Note that the callback is called once for each
          // certificate in the certificate chain, starting from the
          // root certificate authority.

          // In this example we will simply print the certificate's
          // subject name.
          char subject_name[256];
          X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
          SL_TRACE(log, "Verifying {}", subject_name);

          return preverified;
        });

    doConnect();
  }

  void HttpRequest::doConnect() {
    SL_TRACE(log_,
             "Connect to `{}` (addr={}:{})",
             uri_.Host,
             resolver_iterator_->endpoint().address().to_string(),
             resolver_iterator_->endpoint().port());

    boost::asio::async_connect(
        socket_.lowest_layer(),
        resolver_iterator_,
        [wp = weak_from_this()](const boost::system::error_code &ec, auto it) {
          if (auto self = wp.lock()) {
            if (!ec) {
              SL_TRACE(self->log_, "Connection established");
              self->onConnected();
              return;
            }

            SL_ERROR(self->log_, "Connection failed: {}", ec.message());

            // Try to connect next endpoint if any
            if (self->resolver_iterator_
                != boost::asio::ip::tcp::resolver::iterator{}) {
              SL_TRACE(self->log_, "Trying next endpoint...");
              ++self->resolver_iterator_;
              self->doConnect();
            } else {
              self->status_ = HttpStatus(20);
            }
          }
        });
  }

  void HttpRequest::onConnected() {
    doWriting();
  }

  void HttpRequest::doWriting() {
    if (send_buffer_.size() == 0) {
      SL_TRACE(log_, "No data to write");
      return;
    }
    SL_TRACE(log_, "Remaining to write {} bytes", send_buffer_.size());

    if (writing_in_progress_) {
      return;
    }
    writing_in_progress_ = true;

    SL_TRACE(log_, "Try to write {} bytes", send_buffer_.size());

    boost::asio::async_write(
        socket_,
        send_buffer_,
        [wp = weak_from_this()](const boost::system::error_code &ec,
                                std::size_t written) {
          if (auto self = wp.lock()) {
            if (!ec) {
              SL_TRACE(self->log_, "Written {} bytes", written);
              self->send_buffer_.commit(written);
              self->writing_in_progress_ = false;
              self->doWriting();
              return;
            }

            std::cerr << ec.message() << std::endl;
            SL_ERROR(self->log_, "Writing failed: {}", ec.message());
            self->status_ = HttpStatus(20);
          }
        });
  }

  void HttpRequest::onWritten(size_t size) {
    //
  }

  RequestId HttpRequest::id() const {
    return id_;
  }

  HttpStatus HttpRequest::status() const {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::status is not implemented yet");
    return {};
  }

  Result<Success, Failure> HttpRequest::addRequestHeader(
      std::string_view name, std::string_view value) {
    if (not adding_headers_is_allowed_) {
      return Failure();
    }

    {  // Write additional header
      std::ostringstream oss;
      oss << name << ": " << value << "\r\n";
      auto header = oss.str();

      boost::asio::buffer_copy(
          send_buffer_.prepare(header.size()),
          boost::asio::const_buffer(header.data(), header.size()));
      send_buffer_.commit(header.size());
    }

    headers_.emplace_back(name, value);
    return Success();
  }

  Result<Success, HttpError> HttpRequest::writeRequestBody(
      const common::Buffer &chunk, boost::optional<Timestamp> deadline) {
    adding_headers_is_allowed_ = false;

    boost::asio::buffer_copy(
        send_buffer_.prepare(chunk.size()),
        boost::asio::const_buffer(chunk.data(), chunk.size()));
    send_buffer_.commit(chunk.size());

    if (chunk.empty()) {
      if (not body_sent_) {
        doWriting();
      }
      body_sent_ = true;
    }

    return Result<Success, HttpError>();
  }

  std::vector<std::pair<std::string, std::string>>
  HttpRequest::getResponseHeaders() const {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::getResponseHeaders is not implemented yet");
    return std::vector<std::pair<std::string, std::string>>();
  }

  Result<uint32_t, HttpError> HttpRequest::readResponseBody(
      common::Buffer &chunk, boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "Method HttpRequest::readResponseBody is not implemented yet");
    return Result<uint32_t, HttpError>();
  }
}  // namespace kagome::offchain
