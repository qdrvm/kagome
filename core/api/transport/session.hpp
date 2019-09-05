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

    using OnStopped = Signal<void(std::shared_ptr<Session>)>;
    using OnRequest = Signal<void(std::string_view)>;
    using OnResponse = Signal<void(std::string)>;
    using OnError = Signal<void(boost::system::error_code, std::string_view)>;

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
     * @return `on stopped` signal
     */
    auto &onStopped() {
      return on_stopped_;
    }

    /**
     * @return `on request` signal
     */
    auto &onRequest() {
      return on_request_;
    }

    /**
     * @return `on response` signal
     */
    auto &onResponse() {
      return on_response_;
    }

    /**
     * @return `on error` signal
     */
    auto &onError() {
      return on_error_;
    }

   private:
    OnStopped on_stopped_;    ///< `on stopped` signal
    OnRequest on_request_;    ///< `on request` signal
    OnResponse on_response_;  ///< `on response` signal
    OnError on_error_;        ///< `on error` signal
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
