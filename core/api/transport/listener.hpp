/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_LISTENER_HPP
#define KAGOME_CORE_API_TRANSPORT_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <memory>

#include "api/transport/rpc_io_context.hpp"
#include "api/transport/session.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class Listener {
   protected:
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using Signal = boost::signals2::signal<T>;
    using OnError = Signal<void(const outcome::result<void> &)>;

    using NewSessionHandler = std::function<void(const sptr<Session> &)>;

   public:
    using Context = RpcContext;

    virtual ~Listener() = default;

    /// Bind endpoint
    virtual void prepare() = 0;

    /// Start handling inner connection
    virtual void start() = 0;

    /// Stop working
    virtual void stop() = 0;

    /// Set handler for working new session
    virtual void setHandlerForNewSession(
        NewSessionHandler &&on_new_session) = 0;

   protected:
    /// Accept incoming connection
    virtual void acceptOnce() = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_IMPL_HPP
