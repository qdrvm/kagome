/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include <boost/filesystem/path.hpp>

#include "injector/application_injector.hpp"
#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "application/app_state_manager.hpp"
#include "network/router.hpp"
#include "network/peer_manager.hpp"
#include "api/service/api_service.hpp"
#include "common/logger.hpp"

namespace kagome::application {

  class SyncingNodeApplication : public KagomeApplication {
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~SyncingNodeApplication() override = default;

    explicit SyncingNodeApplication(const AppConfiguration &app_config);

    void run() override;

   private:
    common::Logger logger_;

    uptr<injector::SyncingNodeInjector> injector_;
    sptr<AppStateManager> app_state_manager_;
    sptr<boost::asio::io_context> io_context_;
    sptr<network::Router> router_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    sptr<api::ApiService> jrpc_api_service_;
    sptr<ChainSpec> chain_spec_;
    boost::filesystem::path chain_path_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
