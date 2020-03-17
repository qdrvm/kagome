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
    enum class State { READY, WORKING, STOPPED = READY };
    using Context = boost::asio::io_context;
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using Duration = boost::asio::steady_timer::duration;
    using Logger = common::Logger;

   public:
    /***
     * Listener configuration
     */
    struct Configuration {
      Endpoint endpoint{};  ///< listener endpoint
    };

    /**
     * @param context reference to boost::asio::io_context instance
     * @param endpoint loopback ip address to listen
     * @param ws_config http session configuration
     */
    WsListenerImpl(Context &context,
                   const Configuration &configuration,
                   WsSession::Configuration ws_config);

    ~WsListenerImpl() override = default;

    /**
     * @brief starts listener
     * @param on_new_session new session creation callback
     */
    void start(NewSessionHandler on_new_session) override;

    /**
     * @brief stops listener
     */
    void stop() override;

   private:
    /**
     * @brief accepts incoming connection
     */
    void acceptOnce(NewSessionHandler on_new_session) override;

    Context &context_;                      ///< io context
    Acceptor acceptor_;                     ///< connections acceptor
    State state_{State::READY};             ///< working state
    WsSession::Configuration http_config_;  /// http session configuration
    Logger logger_ = common::createLogger("api listener");  ///< logger instance
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP
