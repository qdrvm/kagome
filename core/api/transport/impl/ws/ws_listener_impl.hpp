/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include <atomic>

#include "api/transport/impl/ws/ws_session.hpp"
#include "log/logger.hpp"

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class WsListenerImpl : public Listener,
                         public std::enable_shared_from_this<WsListenerImpl> {
   public:
    using SessionImpl = WsSession;

    WsListenerImpl(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        std::shared_ptr<Context> context,
        Configuration listener_config,
        SessionImpl::Configuration session_config);

    ~WsListenerImpl() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare() override;

    /** @see AppStateManager::takeControl */
    bool start() override;

    /** @see AppStateManager::takeControl */
    void stop() override;

    void setHandlerForNewSession(NewSessionHandler &&on_new_session) override;

   private:
    void acceptOnce() override;

    std::shared_ptr<Context> context_;
    const Configuration config_;
    const SessionImpl::Configuration session_config_;
    const uint32_t max_ws_connections_;

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<NewSessionHandler> on_new_session_;

    std::atomic<Session::SessionId> next_session_id_;
    std::shared_ptr<SessionImpl> new_session_;
    std::atomic<uint32_t> active_connections_;

    log::Logger log_;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_WS_LISTENER_IMPL_HPP
