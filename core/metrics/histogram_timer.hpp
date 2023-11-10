/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "metrics/metrics.hpp"

#include <libp2p/common/final_action.hpp>

namespace kagome::metrics {
  inline std::vector<double> exponentialBuckets(double start,
                                                double factor,
                                                size_t count) {
    std::vector<double> buckets;
    for (auto bucket = start; buckets.size() < count; bucket *= factor) {
      buckets.emplace_back(bucket);
    }
    return buckets;
  }

  struct GaugeHelper {
    GaugeHelper(const std::string &name, const std::string &help) {
      registry_->registerGaugeFamily(name, help);
      metric_ = registry_->registerGaugeMetric(name);
    }

    auto *operator->() {
      return metric_;
    }

    metrics::RegistryPtr registry_ = metrics::createRegistry();
    metrics::Gauge *metric_;
  };

  struct HistogramHelper {
    HistogramHelper(const std::string &name,
                    const std::string &help,
                    std::vector<double> buckets) {
      registry_->registerHistogramFamily(name, help);
      metric_ = registry_->registerHistogramMetric(name, buckets);
    }

    void observe(double value) {
      metric_->observe(value);
    }

    metrics::RegistryPtr registry_ = metrics::createRegistry();
    metrics::Histogram *metric_;
  };

  struct HistogramTimer : HistogramHelper {
    using Clock = std::chrono::steady_clock;
    using Time = Clock::time_point;

    using HistogramHelper::HistogramHelper;

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
      return std::make_optional(::libp2p::common::MovableFinalAction(manual()));
    }
  };
}  // namespace kagome::metrics
