/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics/impl/prometheus/metrics_impl.hpp"

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>

namespace kagome::metrics {
  PrometheusCounter::PrometheusCounter(prometheus::Counter &m) : m_(m) {}

  void PrometheusCounter::inc() {
    m_.Increment();
  }

  void PrometheusCounter::inc(double val) {
    m_.Increment(val);
  }

  PrometheusGauge::PrometheusGauge(prometheus::Gauge &m) : m_(m) {}

  void PrometheusGauge::inc() {
    m_.Increment();
  }

  void PrometheusGauge::inc(double val) {
    m_.Increment(val);
  }

  void PrometheusGauge::dec() {
    m_.Decrement();
  }

  void PrometheusGauge::dec(double val) {
    m_.Decrement(val);
  }

  void PrometheusGauge::set(double val) {
    m_.Set(val);
  }

  PrometheusSummary::PrometheusSummary(prometheus::Summary &m) : m_(m) {}

  void PrometheusSummary::observe(const double value) {
    m_.Observe(value);
  }

  PrometheusHistogram::PrometheusHistogram(prometheus::Histogram &m) : m_(m) {}

  void PrometheusHistogram::observe(const double value) {
    m_.Observe(value);
  }
}  // namespace kagome::metrics
