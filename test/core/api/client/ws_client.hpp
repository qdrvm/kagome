/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "outcome/outcome.hpp"

namespace test {

  enum class WsClientError {
    CONNECTION_FAILED = 1,
    WEBSOCKET_ERROR,
    HTTP_ERROR,
    NETWORK_ERROR
  };

  /**
   * Simple synchronous client for api service
   * it allows making synchronous http queries to api service
   */
  class WsClient {
    using Socket = boost::asio::ip::tcp::socket;
    using FlatBuffer = boost::beast::flat_buffer;
    using HttpField = boost::beast::http::field;
    using HttpError = boost::beast::http::error;
    using HttpMethods = boost::beast::http::verb;
    using StringBody = boost::beast::http::string_body;
    using DynamicBody = boost::beast::http::dynamic_body;
    using QueryCallback = void(outcome::result<std::string>);
    using Context = boost::asio::io_context;

    template <typename Body>
    using HttpRequest = boost::beast::http::request<Body>;

    template <typename Body>
    using HttpResponse = boost::beast::http::response<Body>;

    template <class Body>
    using RequestParser = boost::beast::http::request_parser<Body>;

    static constexpr auto kUserAgent = "Kagome test api client 0.1";

   public:
    /**
     * @param context reference to io context instance
     */
    explicit WsClient(Context &context) : stream_(context) {}

    WsClient(const WsClient &other) = delete;
    WsClient &operator=(const WsClient &other) = delete;
    WsClient(WsClient &&other) = delete;
    WsClient &operator=(WsClient &&other) = delete;
    ~WsClient();

    /**
     * @brief connects to endpoint
     * @param endpoint address to connect
     * @return error code as outcome::result if failed or success
     */
    outcome::result<void> connect(boost::asio::ip::tcp::endpoint endpoint);

    /**
     * @brief make synchronous query to api service
     * @param message api query message
     * @param callback instructions to execute on completion
     */
    void query(std::string_view message,
               std::function<void(outcome::result<std::string>)> &&callback);

    /**
     * @brief disconnects stream
     */
    void disconnect();

   private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> stream_;
    boost::asio::ip::tcp::endpoint endpoint_;
  };
}  // namespace test

OUTCOME_HPP_DECLARE_ERROR(test, WsClientError)
