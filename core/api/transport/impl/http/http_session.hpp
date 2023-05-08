/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP

#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <cstdlib>
#include <memory>

#include "api/allow_unsafe.hpp"
#include "api/transport/session.hpp"
#include "log/logger.hpp"

namespace kagome::api {
  /**
   * @brief HTTP session for api service
   */
  class HttpSession : public Session,
                      public std::enable_shared_from_this<HttpSession> {
    template <typename Body>
    using Request = boost::beast::http::request<Body>;

    template <typename Body>
    using Response = boost::beast::http::response<Body>;

    using StringBody = boost::beast::http::string_body;

    template <class Body>
    using RequestParser = boost::beast::http::request_parser<Body>;

    using HttpField = boost::beast::http::field;

    using HttpError = boost::beast::http::error;

   public:
    struct Configuration {
      static constexpr size_t kDefaultRequestSize = 10000u;
      static constexpr Duration kDefaultTimeout = std::chrono::seconds(30);

      size_t max_request_size{kDefaultRequestSize};
      Duration operation_timeout{kDefaultTimeout};
    };

    ~HttpSession() override = default;

    /**
     * @brief constructor
     * @param socket socket instance
     * @param config session configuration
     */
    HttpSession(Context &context,
                AllowUnsafe allow_unsafe,
                Configuration config);

    Socket &socket() override {
      return stream_.socket();
    }

    /**
     * @brief starts session
     */
    void start() override;

    /**
     * @brief sends response wrapped by http message
     * @param response message to send
     */
    void respond(std::string_view response) override;

    /**
     * @brief method to get id of the session
     * @return id of the session
     */
    SessionId id() const override {
      return 0ull;
    }

    /**
     * @brief method to get type of the session
     * @return type of the session
     */
    SessionType type() const override {
      return SessionType::kHttp;
    }

    void post(std::function<void()> cb) override;

    bool isUnsafeAllowed() const override;

   private:
    /**
     * @brief stops session
     */
    void stop();

    /**
     * @brief process http request, compose and execute response
     * @tparam Body request body type
     * @param request request
     */
    template <class Body>
    void handleRequest(Request<Body> &&request);

    /**
     * @brief asynchronously read http message
     */
    void asyncRead();

    /**
     * @brief sends http message
     * @tparam Message http message type
     * @param message http message
     */
    template <class Message>
    void asyncWrite(Message &&message);

    /**
     * @brief read completion callback
     */
    void onRead(boost::system::error_code ec, std::size_t size);

    /**
     * @brief write completion callback
     */
    void onWrite(boost::system::error_code ec, std::size_t, bool close);

    /**
     * @brief composes `bad request` message
     * @param message text to send
     * @param version protocol version
     * @param keep_alive true if server should keep connection alive, false
     * otherwise
     * @return composed request
     */
    auto makeBadResponse(std::string_view message,
                         unsigned version,
                         bool keep_alive);

    /**
     * @brief reports error code and message
     * @param ec error code
     * @param message error message
     */
    void reportError(boost::system::error_code ec, std::string_view message);

    static constexpr boost::string_view kServerName = "Kagome";

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    AllowUnsafe allow_unsafe_;
    Configuration config_;              ///< session configuration
    boost::beast::tcp_stream stream_;   ///< stream
    boost::beast::flat_buffer buffer_;  ///< read buffer

    /**
     * @brief request parser type
     */
    using Parser = RequestParser<StringBody>;

    std::unique_ptr<Parser> parser_;  ///< http parser
    log::Logger logger_ = log::createLogger("HttpSession", "rpc_transport");
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_BEAST_HTTP_SESSION_HPP
