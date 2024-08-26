/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include "common/uri.hpp"
#include "log/logger.hpp"
#include "offchain/types.hpp"
#include "utils/asio_ssl_context_client.hpp"

namespace kagome::offchain {

  class HttpRequest final : public std::enable_shared_from_this<HttpRequest> {
   public:
    HttpRequest(HttpRequest &&) = delete;
    HttpRequest(const HttpRequest &) = delete;

    HttpRequest(RequestId id);

    RequestId id() const;

    HttpStatus status() const;

    bool init(HttpMethod method, std::string_view uri, common::Buffer meta);

    Result<Success, Failure> addRequestHeader(std::string_view name,
                                              std::string_view value);

    Result<Success, HttpError> writeRequestBody(
        const common::Buffer &chunk,
        std::optional<std::chrono::milliseconds> deadline_opt);

    std::vector<std::pair<std::string, std::string>> getResponseHeaders() const;

    Result<uint32_t, HttpError> readResponseBody(
        common::Buffer &chunk,
        std::optional<std::chrono::milliseconds> deadline);

    std::string errorMessage() const {
      return error_message_;
    }

   private:
    void resolve();
    void connect();
    void handshake();
    void sendRequest();
    void recvResponse();
    void done();

    boost::asio::io_context io_context_;
    int16_t id_;

    boost::asio::ip::tcp::resolver resolver_;
    std::optional<AsioSslContextClient> ssl_ctx_;

    using TcpStream = boost::beast::tcp_stream;
    using SslStream = boost::beast::ssl_stream<TcpStream>;
    using TcpStreamPtr = std::unique_ptr<TcpStream>;
    using SslStreamPtr = std::unique_ptr<SslStream>;
    boost::variant<TcpStreamPtr, SslStreamPtr> stream_;

    common::Uri uri_;
    bool adding_headers_is_allowed_ = true;
    bool request_has_sent_ = false;
    bool secure_ = false;
    uint16_t status_ = 0;
    std::string error_message_;
    boost::beast::flat_buffer buffer_;
    boost::asio::steady_timer deadline_timer_;
    boost::asio::ip::tcp::resolver::iterator resolver_iterator_;
    boost::beast::http::request<boost::beast::http::string_body> request_;
    boost::beast::http::response_parser<boost::beast::http::string_body>
        parser_;
    boost::beast::http::response<boost::beast::http::string_body> response_;
    bool request_is_ready_ = false;
    bool connected_ = false;

    log::Logger log_;
  };

}  // namespace kagome::offchain
