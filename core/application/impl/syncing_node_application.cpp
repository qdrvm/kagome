/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/syncing_node_application.hpp"
#include "network/common.hpp"

namespace kagome::application {

  SyncingNodeApplication::SyncingNodeApplication(
      const std::string &config_path,
      const std::string &leveldb_path,
      uint16_t p2p_port,
      const boost::asio::ip::tcp::endpoint &rpc_http_endpoint,
      const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint,
      uint8_t verbosity)
      : injector_{injector::makeSyncingNodeInjector(config_path,
                                                    leveldb_path,
                                                    p2p_port,
                                                    rpc_http_endpoint,
                                                    rpc_ws_endpoint)},
        logger_{common::createLogger("SyncingNodeApplication")} {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(verbosity));

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    config_storage_ = injector_.create<sptr<ConfigurationStorage>>();
    router_ = injector_.create<sptr<network::Router>>();

    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void SyncingNodeApplication::run() {
    logger_->info("Start as {} with PID {}", typeid(*this).name(), getpid());

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
        for (const auto &boot_node : config_storage_->getBootNodes().peers) {
          host.newStream(
              boot_node,
              network::kGossipProtocol,
              [this, boot_node](const auto &stream_res) {
                if (not stream_res) {
                  this->logger_->error(
                      "Could not establish connection with {}. Error: {}",
                      boot_node.id.toBase58(),
                      stream_res.error().message());
                  return;
                }
                this->router_->handleGossipProtocol(stream_res.value());
              });
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
