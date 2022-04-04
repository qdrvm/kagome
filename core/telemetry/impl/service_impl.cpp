/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/impl/service_impl.hpp"

#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include "telemetry/impl/connection_impl.hpp"

namespace kagome::telemetry {

  TelemetryServiceImpl::TelemetryServiceImpl(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      const application::AppConfiguration &app_configuration,
      const application::ChainSpec &chain_spec,
      const libp2p::Host &host)
      : io_context_{std::move(io_context)},
        app_state_manager_{std::move(app_state_manager)},
        block_tree_{std::move(block_tree)},
        app_configuration_{app_configuration},
        chain_spec_{chain_spec},
        host_{host} {
    BOOST_ASSERT(io_context_);
    BOOST_ASSERT(app_state_manager_);
    BOOST_ASSERT(block_tree_);

    app_state_manager_->takeControl(*this);
  }

  bool TelemetryServiceImpl::prepare() {
    auto &chain_spec = chain_spec_.telemetryEndpoints();
    auto &cli_config = app_configuration_.telemetryEndpoints();
    auto &endpoints = cli_config.empty() ? chain_spec : cli_config;

    for (const auto &endpoint : endpoints) {
      auto connection = std::make_shared<TelemetryConnectionImpl>(
          io_context_,
          endpoint,
          [self{shared_from_this()}](
              std::shared_ptr<TelemetryConnection> conn) {
            conn->send(self->connectedMessage());
          });
      connections_.emplace_back(std::move(connection));
    }

    return true;
  }

  bool TelemetryServiceImpl::start() {
    for (auto &connection : connections_) {
      connection->connect();
    }
    return true;
  }

  void TelemetryServiceImpl::stop() {
    for (auto &connection : connections_) {
      connection->shutdown();
    }
  }

  std::string TelemetryServiceImpl::currentTimestamp() const {
    // todo
    // https://stackoverflow.com/questions/3854496/how-do-i-get-the-current-utc-offset-time-zone
    auto t = boost::posix_time::microsec_clock::universal_time();
    return boost::posix_time::to_iso_extended_string(t) + "+00:00";
  }

  std::string TelemetryServiceImpl::connectedMessage() const {
    return fmt::format(
        "{{\"id\":1,\"payload\":{{\"authority\":{},\"chain\":\"{}\",\"config\":"
        "\"\",\"genesis_hash\":\"0x{}\",\"implementation\":\"{}\",\"msg\":"
        "\"system.connected\",\"name\":\"{}\",\"network_id\":\"{}\",\"startup_"
        "time\":\"{}\",\"version\":\"{}\"}},\"ts\":\"{}\"}}",
        app_configuration_.roles().flags.authority ? "true" : "false",
        chain_spec_.name(),
        block_tree_->getGenesisBlockHash().toHex(),
        "Kagome Node",
        app_configuration_.nodeName(),
        host_.getId().toBase58(),
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count(),
        "0.0.1",  // app_configuration_.nodeVersion(),
        currentTimestamp());
  }

  void TelemetryServiceImpl::blockImported(const std::string &hash,
                                           uint32_t height) {
    if (not random_bool_generator_(rand_engine_)) {
      return;
    }
    auto msg = fmt::format(
        "{{\"id\":1,\"payload\":{{\"best\":\"0x{}\",\"height\":{},\"msg\":"
        "\"block.import\",\"origin\":\"NetworkInitialSync\"}},\"ts\":\"{}\"}}",
        hash,
        height,
        currentTimestamp());

    for (auto &connection : connections_) {
      connection->send(msg);
    }
  }
}  // namespace kagome::telemetry
