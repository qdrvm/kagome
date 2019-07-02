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

    /**
     * @param context reference to boost::asio::io_context instance
     * @param endpoint loopback ip address to listen
     */
    Listener(Context &context, Endpoint endpoint);

    virtual ~Listener() = default;

    auto &onNewSession() {
      return on_new_session_;
    }

    virtual void start() {
      state_ = State::WORKING;
      doAccept();
    }

    virtual void stop() {
      state_ = State::STOPPED;
      acceptor_.cancel();
    }

   private:
    virtual void doAccept();

    Signal<void(sptr<Session>)> on_new_session_;
    Context &context_;
    Acceptor acceptor_;

    State state_{State::READY};
    SessionManager session_manager_;
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_HPP
