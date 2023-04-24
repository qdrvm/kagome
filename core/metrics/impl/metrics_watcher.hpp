/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_METRICS_METRICWATCHER
#define KAGOME_METRICS_METRICWATCHER

#include <thread>
#include <filesystem>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
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

    std::filesystem::path storage_path_;

    volatile bool shutdown_requested_ = false;
    std::thread thread_;

    // Metrics
    metrics::RegistryPtr metrics_registry_;
    metrics::Gauge *metric_storage_size_;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_METRICS_METRICWATCHER
