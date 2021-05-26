/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_IMPL_PROMETHEUS_METRICS_IMPL_HPP
#define KAGOME_CORE_METRICS_IMPL_PROMETHEUS_METRICS_IMPL_HPP

#include "metrics/metrics.hpp"

namespace prometheus {
  class Counter;
  class Gauge;
  class Summary;
  class Histogram;
}  // namespace prometheus

namespace kagome::metrics {
  class PrometheusCounter : public Counter {
    friend class Registry;
    prometheus::Counter &c_;

   public:
    PrometheusCounter(prometheus::Counter &c);

   public:
    void inc() override;
    void inc(double val) override;
    double val() override;
  };

  class PrometheusGauge : public Gauge {
    friend class Registry;
    prometheus::Gauge &g_;

   public:
    PrometheusGauge(prometheus::Gauge &g);

   public:
    void inc() override;
    void inc(double val) override;
    void dec() override;
    void dec(double val) override;
    void set(double val) override;
  };

  class PrometheusSummary : public Summary {
    friend class Registry;
    prometheus::Summary &s_;

   public:
    PrometheusSummary(prometheus::Summary &s);

   public:
    void observe(const double value) override;
  };

  class PrometheusHistogram : public Histogram {
    friend class Registry;
    prometheus::Histogram &h_;

   public:
    PrometheusHistogram(prometheus::Histogram &h);

   public:
    void observe(const double value) override;
  };
}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_IMPL_PROMETHEUS_METRICS_IMPL_HPP
