/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_LISTENER_HPP
#define KAGOME_CORE_API_TRANSPORT_LISTENER_HPP

#include <memory>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "api/transport/session_manager.hpp"

namespace kagome::server {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class Listener {
   protected:
    /**
     * @brief server state
     */
    enum class State { READY, WORKING, STOPPED = READY };
    template <class T>
    using sptr = std::shared_ptr<Session>;

    using Context = boost::asio::io_context;
    using Acceptor = boost::asio::ip::tcp::acceptor;
    template <class T>
    using Signal = boost::signals2::signal<T>;

   public:
    using Endpoint = boost::asio::ip::tcp::endpoint;

    virtual ~Listener() = default;

    inline auto &onNewSession() {
      return on_new_session_;
    }

    inline auto &onError() {
      return on_error_;
    }

    inline auto &onStopped() {
      return on_stopped_;
    }

    /**
     * @brief starts listening
     */
    virtual void start() = 0;

    /**
     * @brief stops listening
     */
    virtual void stop() = 0;

   protected:
    /**
     * @brief accepts incoming connection
     */
    virtual void doAccept() = 0;

    Signal<void(sptr<Session>)>
        on_new_session_;  ///< emitted when new session is created
    Signal<void(outcome::result<void>)>
        on_error_;               ///< emitted when error occurs
    Signal<void()> on_stopped_;  ///< emitted when listener stops
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_IMPL_HPP
