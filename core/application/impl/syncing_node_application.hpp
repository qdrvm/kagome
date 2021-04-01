/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include "injector/syncing_node_injector.hpp"
#include "log/logger.hpp"

namespace kagome::application {

  class SyncingNodeApplication : public KagomeApplication {
    using InjectorType = decltype(injector::makeSyncingNodeInjector(
        std::declval<const AppConfiguration &>()));

    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~SyncingNodeApplication() override = default;

    explicit SyncingNodeApplication(const AppConfiguration &app_config);

    void run() override;

   private:
    const AppConfiguration &app_config_;
    InjectorType injector_;

    std::shared_ptr<soralog::LoggingSystem> logging_system_;
    std::shared_ptr<AppStateManager> app_state_manager_;
    std::shared_ptr<ChainSpec> chain_spec_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    std::shared_ptr<api::ApiService> jrpc_api_service_;
    std::shared_ptr<boost::asio::io_context> io_context_;

    log::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
