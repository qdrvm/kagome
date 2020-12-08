/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/block_producing_node_application.hpp"
#include "application/impl/util.hpp"

namespace kagome::application {

  BlockProducingNodeApplication::BlockProducingNodeApplication(
      const AppConfiguration &app_config)
      : injector_{injector::makeBlockProducingNodeInjector(app_config)},
        logger_(common::createLogger("Application")) {
    spdlog::set_level(app_config.verbosity());

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();
    chain_path_ = app_config.chain_path(genesis_config_->id());

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    genesis_config_ = injector_.create<sptr<ChainSpec>>();
    clock_ = injector_.create<sptr<clock::SystemClock>>();
    babe_ = injector_.create<sptr<Babe>>();
    router_ = injector_.create<sptr<network::Router>>();
    peer_manager_ = injector_.create<sptr<network::PeerManager>>();
    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void BlockProducingNodeApplication::run() {
    logger_->info("Start as BlockProducingNode with PID {}", getpid());

    auto res = util::init_directory(chain_path_);
    if (not res) {
      logger_->critical("Error initalizing chain directory {}: {}",
                        chain_path_.native(),
                        res.error().message());
      exit(EXIT_FAILURE);
    }

    babe_->setExecutionStrategy(Babe::ExecutionStrategy::SYNC_FIRST);

    app_state_manager_->atLaunch([ctx{io_context_}] {
      std::thread asio_runner([ctx{ctx}] { ctx->run(); });
      asio_runner.detach();
      return true;
    });

    app_state_manager_->atShutdown([ctx{io_context_}] { ctx->stop(); });

    app_state_manager_->run();
  }

}  // namespace kagome::application
