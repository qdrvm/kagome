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
  class Session : public std::enable_shared_from_this<Session> {
    template <class T>
    using Signal = boost::signals2::signal<T>;

    using OnRequestSignature = void(std::string_view,
                                    std::shared_ptr<Session> session);
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
    void connectOnRequest(std::function<OnRequestSignature> callback) {
      on_request_ = std::move(callback);
    }

    /**
     * @brief process request message
     * @param request message to process
     */
    void processRequest(std::string_view request,
                        std::shared_ptr<Session> session) {
      on_request_(request, std::move(session));
    }

    /**
     * @brief connects `on response` callback
     * @param callback `on response` callback
     */
    void connectOnResponse(std::function<OnResponseSignature> callback) {
      on_response_ = std::move(callback);
    }

    /**
     * @brief send response message
     * @param message response message
     */
    void respond(std::string_view message) {
      on_response_(message);
    }

    /**
     * @brief connects `on error` callback
     * @param callback `on error` callback
     */
    void connectOnError(std::function<OnErrorSignature> callback) {
      on_error_ = std::move(callback);
    }

    /**
     * @brief reports error code and message
     * @param ec error code
     * @param message error message
     */
    void reportError(boost::system::error_code ec, std::string_view message) {
      on_error_(ec, message);
    }

   private:
    std::function<OnRequestSignature> on_request_;    ///< `on request` callback
    std::function<OnResponseSignature> on_response_;  ///< `on response` signal
    std::function<OnErrorSignature> on_error_;        ///< `on error` signal
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
