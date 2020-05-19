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

    /**
     * @brief starts listening
     */
    virtual void start(NewSessionHandler on_new_session) = 0;

    /**
     * @brief stops listening
     */
    virtual void stop() = 0;

   protected:
    /**
     * @brief accepts incoming connection
     */
    virtual void acceptOnce(NewSessionHandler on_new_session) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_LISTENER_IMPL_HPP
