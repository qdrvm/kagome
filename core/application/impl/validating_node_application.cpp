/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/validating_node_application.hpp"

#include <thread>

#include "application/impl/util.hpp"
#include "injector/application_injector.hpp"
//#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"

namespace kagome::application {

  ValidatingNodeApplication::ValidatingNodeApplication(
      const AppConfiguration &app_config)
      : logger_(log::createLogger("ValidatingNodeApplication", "application")),
        injector_{
            std::make_unique<injector::ValidatingNodeInjector>(app_config)},
        node_name_{app_config.nodeName()} {
    if (app_config.isAlreadySynchronized()) {
      babe_execution_strategy_ =
          consensus::babe::Babe::ExecutionStrategy::START;
    } else {
      babe_execution_strategy_ =
          consensus::babe::Babe::ExecutionStrategy::SYNC_FIRST;
    }

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    chain_spec_ = injector_->injectChainSpec();
    BOOST_ASSERT(chain_spec_ != nullptr);

    app_state_manager_ = injector_->injectAppStateManager();

    chain_path_ = app_config.chainPath(chain_spec_->id());

    io_context_ = injector_->injectIoContext();
    clock_ = injector_->injectSystemClock();
    babe_ = injector_->injectBabe();
    grandpa_ = injector_->injectGrandpa();
    router_ = injector_->injectRouter();
    peer_manager_ = injector_->injectPeerManager();
    jrpc_api_service_ = injector_->injectRpcApiService();
    sync_observer_ = injector_->injectSyncObserver();
  }

  void ValidatingNodeApplication::run() {
    logger_->info("Start as ValidatingNode with PID {} named as {}",
                  getpid(),
                  node_name_);

    auto res = util::init_directory(chain_path_);
    if (not res) {
      logger_->critical("Error initializing chain directory {}: {}",
                        chain_path_.native(),
                        res.error().message());
      exit(EXIT_FAILURE);
    }

    babe_->setExecutionStrategy(babe_execution_strategy_);

    app_state_manager_->atLaunch([ctx{io_context_}] {
      std::thread asio_runner([ctx{ctx}] { ctx->run(); });
      asio_runner.detach();
      return true;
    });

    app_state_manager_->atShutdown([ctx{io_context_}] { ctx->stop(); });

    app_state_manager_->run();
  }

}  // namespace kagome::application
