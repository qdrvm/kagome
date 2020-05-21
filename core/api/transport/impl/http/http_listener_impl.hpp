/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include "api/transport/impl/http/http_session.hpp"
#include "common/logger.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class HttpListenerImpl
      : public Listener,
        public std::enable_shared_from_this<HttpListenerImpl> {
    enum class State { READY, WORKING, STOPPED = READY };
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using Duration = boost::asio::steady_timer::duration;
    using Logger = common::Logger;

   public:
    using SessionImpl = HttpSession;

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
    HttpListenerImpl(std::shared_ptr<Context> context,
                     const Configuration &configuration,
                     SessionImpl::Configuration session_config);

    ~HttpListenerImpl() override = default;

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

    std::shared_ptr<Context> context_;           ///< io context
    Acceptor acceptor_;                          ///< connections acceptor
    State state_{State::READY};                  ///< working state
    SessionImpl::Configuration session_config_;  /// http session configuration

    std::shared_ptr<SessionImpl> new_session_;

    Logger logger_ =
        common::createLogger("RPC_HTTP_Listener");
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
