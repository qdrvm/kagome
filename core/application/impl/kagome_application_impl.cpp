/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"

#include <thread>

#include "application/impl/util.hpp"
#include "consensus/babe/babe.hpp"

namespace kagome::application {

  KagomeApplicationImpl::KagomeApplicationImpl(
      const AppConfiguration &app_config)
      : app_config_(app_config),
        injector_{std::make_unique<injector::KagomeNodeInjector>(app_config)},
        logger_(log::createLogger("Application", "application")),
        node_name_(app_config.nodeName()) {
    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    chain_spec_ = injector_->injectChainSpec();
    BOOST_ASSERT(chain_spec_ != nullptr);

    app_state_manager_ = injector_->injectAppStateManager();

    io_context_ = injector_->injectIoContext();
    clock_ = injector_->injectSystemClock();
    babe_ = injector_->injectBabe();
    exposer_ = injector_->injectOpenMetricsService();
    grandpa_ = injector_->injectGrandpa();
    router_ = injector_->injectRouter();
    peer_manager_ = injector_->injectPeerManager();
    jrpc_api_service_ = injector_->injectRpcApiService();
    sync_observer_ = injector_->injectSyncObserver();
  }

  void KagomeApplicationImpl::run() {
    logger_->info("Start as KagomeApplicationImpl with PID {} named as {}",
                  getpid(),
                  node_name_);

    auto chain_path = app_config_.chainPath(chain_spec_->id());
    auto res = util::init_directory(chain_path);
    if (not res) {
      logger_->critical("Error initializing chain directory {}: {}",
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
