/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <thread>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "filesystem/common.hpp"
#include "metrics/metrics.hpp"
#include "outcome/outcome.hpp"

namespace kagome::metrics {

  class MetricsWatcher final {
   public:
    MetricsWatcher(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::ChainSpec> chain_spec);

    bool start();
    void stop();

   private:
    outcome::result<uintmax_t> measure_storage_size();

    filesystem::path storage_path_;

    volatile bool shutdown_requested_ = false;
    std::thread thread_;

    // Metrics
    metrics::RegistryPtr metrics_registry_;
    metrics::Gauge *metric_storage_size_;
  };

}  // namespace kagome::metrics
