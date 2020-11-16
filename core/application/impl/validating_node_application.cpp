/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/validating_node_application.hpp"

#include <boost/filesystem.hpp>

namespace kagome::application {

  ValidatingNodeApplication::ValidatingNodeApplication(
      const AppConfiguration &app_config)
      : injector_{injector::makeFullNodeInjector(app_config)},
        logger_(common::createLogger("Application")) {
    spdlog::set_level(app_config.verbosity());

    if (app_config.is_already_synchronized()) {
      babe_execution_strategy_ = Babe::ExecutionStrategy::START;
    } else {
      babe_execution_strategy_ = Babe::ExecutionStrategy::SYNC_FIRST;
    }

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    genesis_config_ = injector_.create<sptr<ChainSpec>>();
    clock_ = injector_.create<sptr<clock::SystemClock>>();
    babe_ = injector_.create<sptr<Babe>>();
    grandpa_ = injector_.create<sptr<Grandpa>>();
    router_ = injector_.create<sptr<network::Router>>();

    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void ValidatingNodeApplication::run() {
    logger_->info("Start as {} with PID {}", __PRETTY_FUNCTION__, getpid());

    babe_->setExecutionStrategy(babe_execution_strategy_);

    app_state_manager_->atLaunch([this] {
      // execute listeners
      io_context_->post([this] {
        const auto &current_peer_info =
            injector_.template create<network::OwnPeerInfo>();
        auto &host = injector_.template create<libp2p::Host &>();
        for (const auto &ma : current_peer_info.addresses) {
          auto listen = host.listen(ma);
          if (not listen) {
            logger_->error("Cannot listen address {}. Error: {}",
                           ma.getStringAddress(),
                           listen.error().message());
            std::exit(1);
          }
        }
        this->router_->init();
      });
      return true;
    });

    app_state_manager_->atLaunch([ctx{io_context_}] {
      std::thread asio_runner([ctx{ctx}] { ctx->run(); });
      asio_runner.detach();
      return true;
    });

    app_state_manager_->atShutdown([ctx{io_context_}] { ctx->stop(); });

    app_state_manager_->run();
  }

}  // namespace kagome::application
