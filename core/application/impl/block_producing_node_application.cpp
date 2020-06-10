/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/block_producing_node_application.hpp"

namespace kagome::application {
  using consensus::Epoch;
  using std::chrono_literals::operator""ms;
  using consensus::Randomness;
  using consensus::Threshold;

  BlockProducingNodeApplication::BlockProducingNodeApplication(
      const std::string &config_path,
      const std::string &keystore_path,
      const std::string &leveldb_path,
      uint16_t p2p_port,
      const boost::asio::ip::tcp::endpoint &rpc_http_endpoint,
      const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint,
      uint8_t verbosity)
      : injector_{injector::makeBlockProducingNodeInjector(config_path,
                                                           keystore_path,
                                                           leveldb_path,
                                                           p2p_port,
                                                           rpc_http_endpoint,
                                                           rpc_ws_endpoint)},
        logger_(common::createLogger("Application")) {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(verbosity));

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    config_storage_ = injector_.create<sptr<ConfigurationStorage>>();
    key_storage_ = injector_.create<sptr<KeyStorage>>();
    clock_ = injector_.create<sptr<clock::SystemClock>>();
    babe_ = injector_.create<sptr<Babe>>();
    router_ = injector_.create<sptr<network::Router>>();

    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void BlockProducingNodeApplication::run() {
    logger_->info("Start as {} with PID {}", typeid(*this).name(), getpid());

    app_state_manager_->atLaunch(
        [this] { babe_->start(Babe::ExecutionStrategy::SYNC_FIRST); });

    app_state_manager_->atLaunch([this] {
      // execute listeners
      io_context_->post([this] {
        const auto &current_peer_info =
            injector_.template create<libp2p::peer::PeerInfo>();
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
    });

    app_state_manager_->atLaunch([ctx{io_context_}] {
      std::thread asio_runner([ctx{ctx}] { ctx->run(); });
      asio_runner.detach();
    });

    app_state_manager_->atShutdown([ctx{io_context_}] { ctx->stop(); });

    app_state_manager_->run();
  }
}  // namespace kagome::application
