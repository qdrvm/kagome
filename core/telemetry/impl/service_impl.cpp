/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/impl/service_impl.hpp"

#include "telemetry/impl/connection_impl.hpp"

namespace kagome::telemetry {

  TelemetryServiceImpl::TelemetryServiceImpl(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      const application::AppConfiguration &app_configuration,
      const application::ChainSpec &chain_spec)
      : io_context_{std::move(io_context)},
        app_state_manager_{std::move(app_state_manager)},
        app_configuration_{app_configuration},
        chain_spec_{chain_spec} {
    BOOST_ASSERT(io_context_);
    BOOST_ASSERT(app_state_manager_);

    app_state_manager_->takeControl(*this);
  }

  bool TelemetryServiceImpl::prepare() {
    auto &chain_spec = chain_spec_.telemetryEndpoints();
    auto &cli_config = app_configuration_.telemetryEndpoints();
    auto &endpoints = cli_config.empty() ? chain_spec : cli_config;

    for (const auto &endpoint : endpoints) {
      auto connection = std::make_shared<TelemetryConnectionImpl>(
          io_context_, endpoint, [](auto) {
            std::cout << "Connected!" << std::endl;
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
}  // namespace kagome::telemetry
