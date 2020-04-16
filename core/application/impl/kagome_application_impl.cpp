/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
      : injector_{injector::makeApplicationInjector(config_path,
                                                    keystore_path,
                                                    leveldb_path,
                                                    p2p_port,
                                                    rpc_http_port,
                                                    rpc_ws_port)},
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

    // if we are the first peer in the network, then we get genesis epoch info
    // and start block production
    if (is_genesis_epoch_) {
      // starts block production event loop
      babe_->runGenesisEpoch();
    }
    // starts finalization event loop
    grandpa_launcher_->start();

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

    io_context_->run();
  }
}  // namespace kagome::application
