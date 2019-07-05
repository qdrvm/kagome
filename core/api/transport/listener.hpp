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
#include "api/transport/session.hpp"
#include <outcome/outcome.hpp>

namespace kagome::server {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class Listener {
   protected:
    template <class T>
    using sptr = std::shared_ptr<Session>;

    template <class T>
    using Signal = boost::signals2::signal<T>;
    using OnNewSession = Signal<void(sptr<Session>)>;
    using OnError = Signal<void(outcome::result<void>)>;
    using OnStopped = Signal<void()>;

   public:
    using Endpoint = boost::asio::ip::tcp::endpoint;

    virtual ~Listener() = default;

    /**
     * @return `on new session` signal
     */
    Signal<void(sptr<Session>)> &onNewSession() {
      return on_new_session_;
    }

    /**
     * @return `on error` signal
     */
    Signal<void(outcome::result<void>)> &onError() {
      return on_error_;
    }

    /**
     * @return `on stopped` signal
     */
    Signal<void()> &onStopped() {
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

   private:
    /**
     * @brief accepts incoming connection
     */
    virtual void doAccept() = 0;

    OnNewSession on_new_session_;  ///< emitted when new session is created
    OnError on_error_;             ///< emitted when error occurs
    OnStopped on_stopped_;         ///< emitted when listener stops
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_IMPL_HPP
