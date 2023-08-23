/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_METRICS_HISTOGRAM_TIMER_HPP
#define KAGOME_METRICS_HISTOGRAM_TIMER_HPP

#include "metrics/metrics.hpp"

namespace kagome::metrics {
  struct HistogramTimer {
    using Clock = std::chrono::steady_clock;
    using Time = Clock::time_point;

    HistogramTimer(const std::string &name,
                   const std::string &help,
                   std::vector<double> buckets) {
      registry_->registerHistogramFamily(name, help);
      metric_ = registry_->registerHistogramMetric(name, buckets);
    }

    auto observe(const Time &begin) {
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - begin);
      metric_->observe(ms.count() / 1000.0);
      return ms;
    }

    auto manual() {
      return [this, begin = Clock::now()] { return observe(begin); };
    }

    auto timer() {
      return std::make_optional(gsl::finally(manual()));
    }

    metrics::RegistryPtr registry_ = metrics::createRegistry();
    metrics::Histogram *metric_;
  };
}  // namespace kagome::metrics

#endif  // KAGOME_METRICS_HISTOGRAM_TIMER_HPP
