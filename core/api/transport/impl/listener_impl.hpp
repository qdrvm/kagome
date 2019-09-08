/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include "api/transport/impl/http_session.hpp"

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class ListenerImpl : public Listener,
                       public std::enable_shared_from_this<ListenerImpl> {
    enum class State { READY, WORKING, STOPPED = READY };
    using Context = boost::asio::io_context;
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using Duration = boost::asio::steady_timer::duration;

   public:
    struct Configuration {
      Duration operation_timeout;
    };

    /**
     * @param context reference to boost::asio::io_context instance
     * @param endpoint loopback ip address to listen
     * @param http_config http session configuration
     */
    ListenerImpl(Context &context,
                 const Endpoint &endpoint,
                 HttpSession::Configuration http_config);

    ~ListenerImpl() override = default;

    /**
     * @brief starts listener
     * @param on_new_session new session creation callback
     */
    void start(NewSessionHandler on_new_session) override;

    /**
     * @brief stops listener
     */
    void stop() override;

    /**
     * @return reference to http configuration
     */
    const auto &httpConfig() const {
      return http_config_;
    }

   private:
    /**
     * @brief accepts incoming connection
     */
    void acceptOnce(NewSessionHandler on_new_session) override;

    Context &context_;   ///< io context
    Acceptor acceptor_;  ///< connections acceptor
    // TODO(yuraz): pre-230 add logger and logging in case of errors

    State state_{State::READY};               ///< working state
    HttpSession::Configuration http_config_;  /// http session configuration
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
