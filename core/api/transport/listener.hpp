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
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;

   public:
    using NewSessionHandler =
        std::function<void(const std::shared_ptr<Session> &)>;

    using Context = RpcContext;

    virtual ~Listener() = default;

    /**
     * @brief Bind endpoint
     * @see AppStateManager::takeControl
     */
    virtual bool prepare() = 0;

    /**
     * @brief Start handling inner connection
     * @see AppStateManager::takeControl
     */
    virtual bool start() = 0;

    /**
     * @brief Stop working
     * @see AppStateManager::takeControl
     */
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
