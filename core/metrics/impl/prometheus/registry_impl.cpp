/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics/impl/prometheus/registry_impl.hpp"

#include "metrics/handler.hpp"
#include "metrics/registry.hpp"

namespace kagome::metrics {

  RegistryPtr createRegistry() {
    return std::make_unique<PrometheusRegistry>();
  }

  void PrometheusRegistry::setHandler(Handler &handler) {
    handler.registerCollectable(*this);
  }

  void PrometheusRegistry::registerCounterFamily(
      const std::string &name,
      const std::string &help,
      const std::map<std::string, std::string> &labels) {
    registerFamily<Counter>(name, help, labels);
  }

  void PrometheusRegistry::registerGaugeFamily(
      const std::string &name,
      const std::string &help,
      const std::map<std::string, std::string> &labels) {
    registerFamily<Gauge>(name, help, labels);
  }

  void PrometheusRegistry::registerHistogramFamily(
      const std::string &name,
      const std::string &help,
      const std::map<std::string, std::string> &labels) {
    registerFamily<Histogram>(name, help, labels);
  }

  void PrometheusRegistry::registerSummaryFamily(
      const std::string &name,
      const std::string &help,
      const std::map<std::string, std::string> &labels) {
    registerFamily<Summary>(name, help, labels);
  }

  Counter *PrometheusRegistry::registerCounterMetric(
      const std::string &name,
      const std::map<std::string, std::string> &labels) {
    return registerMetric<Counter>(name, labels);
  }

  Gauge *PrometheusRegistry::registerGaugeMetric(
      const std::string &name,
      const std::map<std::string, std::string> &labels) {
    return registerMetric<Gauge>(name, labels);
  }

  Histogram *PrometheusRegistry::registerHistogramMetric(
      const std::string &name,
      const std::vector<double> &bucket_boundaries,
      const std::map<std::string, std::string> &labels) {
    return registerMetric<Histogram>(name, labels, bucket_boundaries);
  }

  Summary *PrometheusRegistry::registerSummaryMetric(
      const std::string &name,
      const std::vector<std::pair<double, double>> &quantiles,
      std::chrono::milliseconds max_age,
      int age_buckets,
      const std::map<std::string, std::string> &labels) {
    prometheus::Summary::Quantiles q;
    for (const auto &p : quantiles) {
      q.push_back({p.first, p.second});
    }
    return registerMetric<Summary>(name, labels, q, max_age, age_buckets);
  }

}  // namespace kagome::metrics
