// Copyright © 2018 Webgames. All rights reserved.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: d.khaustov@corpwebgames.com
// File created on: 2020.03.13 13:45

// ws_listener_impl.hpp

#pragma once

#include "api/transport/listener.hpp"

#include "common/logger.hpp"
#include "ws_session.hpp"

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
     * @param http_config http session configuration
     */
    WsListenerImpl(Context &context,
                   const Configuration &configuration,
                   WsSession::Configuration http_config);

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
