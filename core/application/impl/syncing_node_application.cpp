/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/syncing_node_application.hpp"

#include "application/impl/util.hpp"
#include "network/common.hpp"

namespace kagome::application {

  SyncingNodeApplication::SyncingNodeApplication(
      const AppConfiguration &app_config)
      : app_config_(app_config),
        injector_{injector::makeSyncingNodeInjector(app_config_)},
        logger_(log::createLogger("SyncingNodeApplication", "application")) {
    log::setLevelOfGroup("main", app_config.verbosity());

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    chain_spec_ = injector_.create<sptr<ChainSpec>>();
    BOOST_ASSERT(chain_spec_ != nullptr);

    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    router_ = injector_.create<sptr<network::Router>>();
    peer_manager_ = injector_.create<sptr<network::PeerManager>>();
    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void SyncingNodeApplication::run() {
    logger_->info("Start as SyncingNode with PID {}", getpid());

    auto chain_path = app_config_.chainPath(chain_spec_->id());
    auto res = util::init_directory(chain_path);
    if (not res) {
      logger_->critical("Error initalizing chain directory {}: {}",
                        chain_path.native(),
                        res.error().message());
      exit(EXIT_FAILURE);
    }

    app_state_manager_->atLaunch([ctx{io_context_}] {
      std::thread asio_runner([ctx{ctx}] { ctx->run(); });
      asio_runner.detach();
      return true;
    });

    app_state_manager_->atShutdown([ctx{io_context_}] { ctx->stop(); });

    app_state_manager_->run();
  }

}  // namespace kagome::application
