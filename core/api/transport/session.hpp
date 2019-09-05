/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_SESSION_HPP
#define KAGOME_CORE_API_TRANSPORT_SESSION_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/signals2/signal.hpp>

namespace kagome::api {
  /**
   * @brief rpc session
   */
  class Session {
    template <class T>
    using Signal = boost::signals2::signal<T>;

    using OnRequestSignature = void(std::string_view);
    using OnRequest = Signal<OnRequestSignature>;
    using OnResponseSignature = void(std::string_view);
    using OnResponse = Signal<OnResponseSignature>;
    using OnErrorSignature = void(boost::system::error_code, std::string_view);
    using OnError = Signal<OnErrorSignature>;

   public:
    using Socket = boost::asio::ip::tcp::socket;
    using Context = boost::asio::io_context;
    using ErrorCode = boost::system::error_code;
    using Streambuf = boost::asio::streambuf;
    using Timer = boost::asio::steady_timer;
    using Connection = boost::signals2::connection;
    using Duration = Timer::duration;

    virtual ~Session() = default;

    /**
     * @brief starts listening on socket
     */
    virtual void start() = 0;

    /**
     * @brief closes connection and session
     */
    virtual void stop() = 0;

    /**
     * @brief connects `on request` callback
     * @param callback `on request` callback
     */
    void connectOnRequest(const std::function<OnRequestSignature> &callback) {
      on_request_cnn_ = on_request_.connect(callback);
    }

    /**
     * @brief process request message
     * @param request message to process
     */
    void processRequest(std::string_view request) {
      on_request_(request);
    }

    /**
     * @brief connects `on response` callback
     * @param callback `on response` callback
     */
    void connectOnResponse(const std::function<OnResponseSignature> &callback) {
      on_response_cnn_ = on_response_.connect(callback);
    }

    void respond(std::string_view message) {
      on_response_(message);
    }

    /**
     * @brief connects `on error` callback
     * @param callback `on error` callback
     */
    void connectOnError(const std::function<OnErrorSignature> &callback) {
      on_error_cnn_ = on_error_.connect(callback);
    }

    void reportError(boost::system::error_code ec, std::string_view message) {
      on_error_(ec, message);
    }

   private:
    OnRequest on_request_;        ///< `on request` signal
    Connection on_request_cnn_;   ///< `on request` cnn holder
    OnResponse on_response_;      ///< `on response` signal
    Connection on_response_cnn_;  ///< `on response` cnn holder
    OnError on_error_;            ///< `on error` signal
    Connection on_error_cnn_;     ///< `on error` cnn holder
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
