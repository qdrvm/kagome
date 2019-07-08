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

namespace kagome::server {
  /**
   * @brief rpc session
   */
  class Session : public std::enable_shared_from_this<Session> {
    template <class T>
    using Signal = boost::signals2::signal<T>;

    using OnStopped = Signal<void(std::shared_ptr<Session>)>;
    using OnRequest =
        Signal<void(std::shared_ptr<Session>, std::string)>;
    using OnResponse = Signal<void(std::string)>;

   public:
    using Socket = boost::asio::ip::tcp::socket;
    using Context = boost::asio::io_context;
    using ErrorCode = boost::system::error_code;
    using Streambuf = boost::asio::streambuf;
    using Timer = boost::asio::steady_timer;
    using Connection = boost::signals2::connection;

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
    inline auto &onStopped() {
      return on_stopped_;
    }

    /**
     * @return `on request` signal
     */
    inline auto &onRequest() {
      return on_request_;
    }

    /**
     * @return `on response` signal
     */
    inline auto &onResponse() {
      return on_response_;
    }

   private:
    OnStopped on_stopped_;    ///< `on stopped` signal
    OnRequest on_request_;    ///< `on request` signal
    OnResponse on_response_;  ///< `on response` signal
  };

}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_SESSION_HPP
