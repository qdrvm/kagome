/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include "api/transport/impl/ws/ws_session.hpp"
#include "common/logger.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class WsListenerImpl : public Listener,
                         public std::enable_shared_from_this<WsListenerImpl> {
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;

   public:
    using SessionImpl = WsSession;

    // TODO(xDimon): Replace value by macro from special generated .h config
    static const uint16_t defaultPort = 40364;

    struct Configuration {
      Endpoint endpoint{};  ///< listener endpoint
      Configuration() {
        endpoint.address(boost::asio::ip::address_v4::any());
        endpoint.port(defaultPort);
      }
    };

    WsListenerImpl(std::shared_ptr<Context> context,
                   const Configuration &listener_config,
                   SessionImpl::Configuration session_config);

    ~WsListenerImpl() override = default;

    void prepare() override;
    void start() override;
    void stop() override;

    void setHandlerForNewSession(NewSessionHandler &&on_new_session) override;

   private:
    void acceptOnce() override;

    std::shared_ptr<Context> context_;
    const Configuration config_;
    const SessionImpl::Configuration session_config_;

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<NewSessionHandler> on_new_session_;

    std::shared_ptr<SessionImpl> new_session_;

    common::Logger logger_ = common::createLogger("RPC_Websocket_Listener");
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP
