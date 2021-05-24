#include "metrics_impl.hpp"
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>

namespace kagome::metrics::lib {
  PrometheusCounter::PrometheusCounter(prometheus::Counter &c) : c_(c) {}

  void PrometheusCounter::inc() {
    c_.Increment();
  }

  void PrometheusCounter::inc(double val) {
    c_.Increment(val);
  }

  double PrometheusCounter::val() {
    return c_.Value();
  }

  PrometheusGauge::PrometheusGauge(prometheus::Gauge &g) : g_(g) {}

  void PrometheusGauge::inc() {
    g_.Increment();
  }

  void PrometheusGauge::inc(double val) {
    g_.Increment(val);
  }

  void PrometheusGauge::dec() {
    g_.Decrement();
  }

  void PrometheusGauge::dec(double val) {
    g_.Decrement(val);
  }

  void PrometheusGauge::set(double val) {
    g_.Set(val);
  }

  PrometheusSummary::PrometheusSummary(prometheus::Summary &s) : s_(s) {}

  void PrometheusSummary::observe(const double value) {
    s_.Observe(value);
  }

  PrometheusHistogram::PrometheusHistogram(prometheus::Histogram &h) : h_(h) {}

  void PrometheusHistogram::observe(const double value) {
    h_.Observe(value);
  }
}  // namespace kagome::metrics::lib
