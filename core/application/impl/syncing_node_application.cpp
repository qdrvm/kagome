/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/syncing_node_application.hpp"

#include <libp2p/log/configurator.hpp>

#include "application/impl/util.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"
#include "network/common.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"

namespace kagome::application {

  SyncingNodeApplication::SyncingNodeApplication(
      const AppConfiguration &app_config)
      : injector_{std::make_unique<injector::SyncingNodeInjector>(app_config)} {
    {
      auto logging_system = std::make_shared<soralog::LoggingSystem>(
          std::make_shared<kagome::log::Configurator>(
              std::make_shared<libp2p::log::Configurator>()));

      auto r = logging_system->configure();
      if (not r.message.empty()) {
        (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
      }
      if (r.has_error) {
        exit(EXIT_FAILURE);
      }

      log::setLoggingSystem(logging_system);
      log::setLevelOfGroup("main", app_config.verbosity());
    }

    logger_ = log::createLogger("SyncingNodeApplication", "application");

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    chain_spec_ = injector_->injectChainSpec();
    BOOST_ASSERT(chain_spec_ != nullptr);

    app_state_manager_ = injector_->injectAppStateManager();

    chain_path_ = app_config.chainPath(chain_spec_->id());
    io_context_ = injector_->injectIoContext();
    router_ = injector_->injectRouter();
    peer_manager_ = injector_->injectPeerManager();
    jrpc_api_service_ = injector_->injectRpcApiService();
  }

  void SyncingNodeApplication::run() {
    logger_->info("Start as SyncingNode with PID {}", getpid());

    auto res = util::init_directory(chain_path_);
    if (not res) {
      logger_->critical("Error initalizing chain directory {}: {}",
                        chain_path_.native(),
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
