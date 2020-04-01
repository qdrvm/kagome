/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"

namespace kagome::application {
  using consensus::Epoch;
  using std::chrono_literals::operator""ms;
  using consensus::Randomness;
  using consensus::Threshold;

  KagomeApplicationImpl::KagomeApplicationImpl(const std::string &config_path,
                                               const std::string &keystore_path,
                                               const std::string &leveldb_path,
                                               uint16_t p2p_port,
                                               uint16_t rpc_http_port,
                                               uint16_t rpc_ws_port,
                                               bool is_genesis_epoch,
                                               uint8_t verbosity)
      : injector_{injector::makeApplicationInjector(
            config_path, keystore_path, leveldb_path, p2p_port, rpc_http_port, rpc_ws_port)},
        is_genesis_epoch_{is_genesis_epoch},
        logger_(common::createLogger("Application")) {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(verbosity));

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    io_context_ = injector_.create<sptr<boost::asio::io_context>>();
    config_storage_ = injector_.create<sptr<ConfigurationStorage>>();
    key_storage_ = injector_.create<sptr<KeyStorage>>();
    clock_ = injector_.create<sptr<clock::SystemClock>>();
    babe_ = injector_.create<sptr<Babe>>();
    grandpa_launcher_ = injector_.create<sptr<GrandpaLauncher>>();
    router_ = injector_.create<sptr<network::Router>>();
    jrpc_api_service_ = injector_.create<sptr<api::ApiService>>();
  }

  void KagomeApplicationImpl::run() {
    jrpc_api_service_->start();
    if (is_genesis_epoch_) {
      babe_->runGenesisEpoch();
    }
    grandpa_launcher_->start();
    io_context_->post([this] {
      const auto &current_peer_info =
          injector_.template create<libp2p::peer::PeerInfo>();
      auto &host = injector_.template create<libp2p::Host &>();
      for (const auto &ma : current_peer_info.addresses) {
        bool listen = host.listen(ma).has_value();
        if (not listen) {
          BOOST_ASSERT(false);
        }
      }
      this->router_->init();
    });

    io_context_->run();
  }
}  // namespace kagome::application
