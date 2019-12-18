/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_API_CLIENT_API_CLIENT_HPP
#define KAGOME_TEST_CORE_API_CLIENT_API_CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <outcome/outcome.hpp>

namespace test {

  enum class ApiClientError {
    CONNECTION_FAILED = 1,
    HTTP_ERROR,
    NETWORK_ERROR
  };

  /**
   * Simple synchronous client for api service
   * it allows making synchronous http queries to api service
   */
  class ApiClient {
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
    explicit ApiClient(Context &context)
        : stream_(context) {}

    ApiClient(const ApiClient &other) = delete;
    ApiClient &operator=(const ApiClient &other) = delete;
    ApiClient(ApiClient &&other) noexcept = delete;
    ApiClient &operator=(ApiClient &&other) noexcept = delete;
    ~ApiClient();

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
    boost::beast::tcp_stream stream_;
    boost::asio::ip::tcp::endpoint endpoint_;
  };
}  // namespace test

OUTCOME_HPP_DECLARE_ERROR(test, ApiClientError)

#endif  // KAGOME_TEST_CORE_API_CLIENT_API_CLIENT_HPP
