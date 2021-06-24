/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_SESSION_HPP
#define KAGOME_CORE_METRICS_SESSION_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http/string_body.hpp>

namespace kagome::metrics {

  /**
   * @brief session interface for OpenMetrics service
   */
  class Session {
   public:
    using Body = boost::beast::http::string_body;
    using Request = boost::beast::http::request<Body>;
    using Response = boost::beast::http::response<Body>;
    using Context = boost::asio::io_context;
    using Socket = boost::asio::ip::tcp::socket;
    using ErrorCode = boost::system::error_code;
    using Streambuf = boost::asio::streambuf;
    using Duration = boost::asio::steady_timer::duration;
    using SessionId = uint64_t;

   private:
    using OnRequestSignature = void(Request, std::shared_ptr<Session> session);
    std::function<OnRequestSignature> on_request_;  ///< `on request` callback

   public:
    struct Configuration {
      static constexpr size_t kDefaultRequestSize = 10000u;
      static constexpr Duration kDefaultTimeout = std::chrono::seconds(30);

      size_t max_request_size{kDefaultRequestSize};
      Duration operation_timeout{kDefaultTimeout};
    };

    virtual ~Session() = default;

    virtual void start() = 0;

    virtual Socket &socket() = 0;

    virtual SessionId id() const = 0;

    /**
     * @brief connects `on request` callback
     */
    void connectOnRequest(std::function<OnRequestSignature> callback) {
      on_request_ = std::move(callback);
    }

    /**
     * @brief process request message
     */
    void processRequest(Request request, std::shared_ptr<Session> session) {
      on_request_(request, std::move(session));
    }

    /**
     * @brief send response message
     */
    virtual void respond(Response message) = 0;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_SESSION_HPP
