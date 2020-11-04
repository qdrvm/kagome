/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/syncing_node_application.hpp"
#include "network/common.hpp"
#include "application/impl/util.hpp"

namespace kagome::application {

  SyncingNodeApplication::SyncingNodeApplication(
      const AppConfiguration &app_config)
      : injector_{injector::makeSyncingNodeInjector(app_config)},
        logger_{common::createLogger("SyncingNodeApplication")} {
    spdlog::set_level(app_config.verbosity());

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    app_state_manager_ = injector_.create<std::shared_ptr<AppStateManager>>();

    genesis_config_ = injector_.create<sptr<ChainSpec>>();

    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    router_ = injector_.create<sptr<network::Router>>();
    chain_path_ = app_config.chain_path(genesis_config_->id());
    kademlia_ =
        injector_
            .create<std::shared_ptr<libp2p::protocol::kademlia::Kademlia>>();

    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void SyncingNodeApplication::run() {
    logger_->info("Start as {} with PID {}", typeid(*this).name(), getpid());

    auto res = util::init_directory(chain_path_);
    if (not res) {
      logger_->error("Error initalizing chain directory: ", res.error().message());
    }

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
        for (const auto &boot_node : genesis_config_->getBootNodes().peers) {
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

        for (const auto &boot_node : config_storage_->getBootNodes().peers) {
          kademlia_->addPeer(boot_node, true);
        }
        kademlia_->start();

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
