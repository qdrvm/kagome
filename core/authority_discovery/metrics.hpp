/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "metrics/metrics.hpp"

namespace kagome::authority_discovery {
  class MetricDhtEventReceived {
   public:
    // clang-tidy cppcoreguidelines-special-member-functions
    ~MetricDhtEventReceived() = default;
    MetricDhtEventReceived(MetricDhtEventReceived &&) = delete;
    MetricDhtEventReceived(const MetricDhtEventReceived &) = delete;
    MetricDhtEventReceived &operator=(MetricDhtEventReceived &&) = delete;
    MetricDhtEventReceived &operator=(const MetricDhtEventReceived &) = delete;

    static auto &get() {
      static MetricDhtEventReceived self;
      return self;
    }
    void getResult(bool found) {
      (found ? value_found : value_not_found)->inc();
    }
    void putResult(bool ok) {
      (ok ? value_put : value_put_failed)->inc();
    }

   private:
    MetricDhtEventReceived() = default;

    static metrics::Counter *make(const std::string &label) {
      auto name = "kagome_authority_discovery_dht_event_received";
      auto help = "Number of dht events received by authority discovery.";
      static auto registry = [&] {
        auto registry = metrics::createRegistry();
        registry->registerCounterFamily(name, help);
        return registry;
      }();
      return registry->registerCounterMetric(name, {{"name", label}});
    }

    metrics::Counter *value_found = make("value_found");
    metrics::Counter *value_not_found = make("value_not_found");
    metrics::Counter *value_put = make("value_put");
    metrics::Counter *value_put_failed = make("value_put_failed");
  };
}  // namespace kagome::authority_discovery
