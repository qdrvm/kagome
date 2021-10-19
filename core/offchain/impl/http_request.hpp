/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_HTTPREQUEST
#define KAGOME_OFFCHAIN_HTTPREQUEST

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>

#include "log/logger.hpp"
#include "offchain/impl/uri.hpp"
#include "offchain/types.hpp"

namespace kagome::offchain {

  class HttpRequest final : public std::enable_shared_from_this<HttpRequest> {
   public:
    HttpRequest(HttpRequest &&) noexcept = delete;
    HttpRequest(const HttpRequest &) = delete;

    HttpRequest(boost::asio::io_context &io_context, RequestId id);

    RequestId id() const;

    HttpStatus status() const;

    bool init(Method method, std::string_view uri, common::Buffer meta);

    Result<Success, Failure> addRequestHeader(std::string_view name,
                                              std::string_view value);

    Result<Success, HttpError> writeRequestBody(
        const common::Buffer &chunk, boost::optional<Timestamp> deadline);

    std::vector<std::pair<std::string, std::string>> getResponseHeaders() const;

    Result<uint32_t, HttpError> readResponseBody(
        common::Buffer &chunk, boost::optional<Timestamp> deadline);

   private:
    void doResolving();
    void onResolved();
    void doConnect();
    void onConnected();
    void doWriting();
    void onWritten(size_t size);

    boost::asio::io_context &io_context_;
    int16_t id_;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ssl::context ssl_ctx_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::beast::tcp_stream socket2_;
    Method method_ = Method::UNDEFINED;
    Uri uri_;
    boost::beast::flat_buffer send_buffer_;
    std::vector<std::pair<std::string, std::string>> headers_;
    bool adding_headers_is_allowed_ = false;
    bool body_sent_ = false;
    uint16_t status_ = 0;
    bool secure_;
    boost::asio::ip::tcp::resolver::iterator resolver_iterator_;
    bool writing_in_progress_ = false;
    log::Logger log_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_HTTPREQUEST
