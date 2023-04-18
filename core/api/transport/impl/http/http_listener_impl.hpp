/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

#include "api/transport/impl/http/http_session.hpp"
#include "log/logger.hpp"

namespace kagome::application {
  class AppStateManager;
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::api {
  /**
   * @brief server which listens for incoming connection,
   * accepts connections making session from socket
   */
  class HttpListenerImpl
      : public Listener,
        public std::enable_shared_from_this<HttpListenerImpl> {
   public:
    using SessionImpl = HttpSession;

    HttpListenerImpl(application::AppStateManager &app_state_manager,
                     std::shared_ptr<Context> context,
                     const application::AppConfiguration &app_config,
                     SessionImpl::Configuration session_config);

    ~HttpListenerImpl() override = default;

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
    const Endpoint endpoint_;
    const SessionImpl::Configuration session_config_;

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<NewSessionHandler> on_new_session_;

    std::shared_ptr<SessionImpl> new_session_;

    log::Logger logger_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_IMPL_LISTENER_IMPL_HPP
