/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "state_metrics/state_metrics.hpp"

#include "api/service/state/state_api.hpp"
#include "application/app_configuration.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "metrics/registry.hpp"
#include "primitives/account.hpp"

#include "libp2p/basic/scheduler.hpp"

#include <thread>
namespace kagome::state_metrics {

  class StateMetricsImpl
      : public StateMetrics,
        public std::enable_shared_from_this<StateMetricsImpl> {
   public:
    StateMetricsImpl(std::string &&validator_address,
                     std::shared_ptr<libp2p::basic::Scheduler> scheduler,
                     std::shared_ptr<api::StateApi> state_api,
                     std::shared_ptr<metrics::Registry> registry,
                     std::shared_ptr<crypto::Hasher> hasher);
    ~StateMetricsImpl() override;

    static outcome::result<std::shared_ptr<StateMetricsImpl>> create(
        const application::AppConfiguration &app_config,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<api::StateApi> state_api,
        std::shared_ptr<metrics::Registry> registry,
        std::shared_ptr<crypto::Hasher> hasher);

    void updateEraPoints() override;

   private:
    std::optional<uint32_t> getActiveEraIndex();
    std::optional<uint32_t> getRewardPoints(uint32_t era_index);
    std::optional<uint32_t> parseErasRewardPoints(
        const std::vector<uint8_t> &data);
    std::vector<uint8_t> generateRewardPointsKey(uint32_t era_index);

    primitives::AccountId validator_id_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<api::StateApi> state_api_;
    kagome::metrics::Gauge *active_era_number_;
    kagome::metrics::Gauge *era_points_;
    std::atomic_bool stop_signal_received_;
    std::thread era_points_thread_;
    log::Logger logger_;
    std::vector<uint8_t> active_era_storage_key_;
    std::vector<uint8_t> reward_points_storage_key_basis_;
  };

}  // namespace kagome::state_metrics
