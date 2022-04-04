/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TELEMETRY_SERVICE_IMPL_HPP
#define KAGOME_TELEMETRY_SERVICE_IMPL_HPP

#include "telemetry/service.hpp"

#include <memory>
#include <vector>

#include <boost/asio/io_context.hpp>
#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "telemetry/connection.hpp"

namespace kagome::telemetry {

  class TelemetryServiceImpl
      : public TelemetryService,
        public std::enable_shared_from_this<TelemetryService> {
   public:
    TelemetryServiceImpl(
        std::shared_ptr<boost::asio::io_context> io_context,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const application::AppConfiguration &app_configuration,
        const application::ChainSpec &chain_spec);

    bool prepare() override;

    bool start() override;

    void stop() override;

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<application::AppStateManager> app_state_manager_;
    const application::AppConfiguration &app_configuration_;
    const application::ChainSpec &chain_spec_;
    std::vector<std::shared_ptr<TelemetryConnection>> connections_;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_TELEMETRY_SERVICE_IMPL_HPP
