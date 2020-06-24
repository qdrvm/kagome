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
    using NewSessionHandler =
        std::function<void(const std::shared_ptr<Session> &)>;

   public:
    using Context = RpcContext;

    struct Configuration {
      Endpoint endpoint{};  ///< listning endpoint
      Configuration() {
        endpoint.address(boost::asio::ip::address_v4::any());
        endpoint.port(0);
      }
    };

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
