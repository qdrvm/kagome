/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_IMPL_PROMETHEUS_REGISTRY_IMPL_HPP
#define KAGOME_CORE_METRICS_IMPL_PROMETHEUS_REGISTRY_IMPL_HPP

#include <forward_list>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include "metrics/impl/prometheus/metrics_impl.hpp"

namespace kagome::metrics {
  class Handler;

  namespace {
    // Helper template structure with specializations
    // that maps metric interfaces to:
    // * prometheus countertypes
    // * implementations
    // * index in Registry::metrics_ tuple
    template <typename T>
    struct MetricInfo {
      using type = prometheus::Counter;
      using dtype = PrometheusCounter;
      static const unsigned index = 0;
    };

    template <>
    struct MetricInfo<Counter> {
      using type = prometheus::Counter;
      using dtype = PrometheusCounter;
      static const unsigned index = 0;
    };

    template <>
    struct MetricInfo<Gauge> {
      using type = prometheus::Gauge;
      using dtype = PrometheusGauge;
      static const unsigned index = 1;
    };

    template <>
    struct MetricInfo<Histogram> {
      using type = prometheus::Histogram;
      using dtype = PrometheusHistogram;
      static const unsigned index = 2;
    };

    template <>
    struct MetricInfo<Summary> {
      using type = prometheus::Summary;
      using dtype = PrometheusSummary;
      static const unsigned index = 3;
    };

  }  // namespace

  class PrometheusRegistry : public Registry {
    friend class PrometheusHandler;
    // prometheus owns families, returns reference
    // sort of cache, not to search every time in prometheus vector of families
    std::unordered_map<std::string,
                       std::reference_wrapper<prometheus::Collectable>>
        family_;
    // main storage of metrics
    std::tuple<std::forward_list<PrometheusCounter>,
               std::forward_list<PrometheusGauge>,
               std::forward_list<PrometheusHistogram>,
               std::forward_list<PrometheusSummary>>
        metrics_;

    // create family for metrics type
    template <typename T>
    void registerFamily(const std::string &name,
                        const std::string &help = "",
                        const std::map<std::string, std::string> &labels = {}) {
      family_.emplace(
          name,
          prometheus::detail::Builder<typename MetricInfo<T>::type>()
              .Name(name)
              .Help(help)
              .Labels(labels)
              .Register(*registry().get()));
    }

    template <typename T, typename... Args>
    T *registerMetric(const std::string &name,
                      const std::map<std::string, std::string> &labels,
                      Args... args) {
      auto &var =
          dynamic_cast<prometheus::Family<typename MetricInfo<T>::type> &>(
              family_.at(name).get())
              .Add(labels, args...);
      return &std::get<MetricInfo<T>::index>(metrics_).emplace_front(
          typename MetricInfo<T>::dtype(var));
    }

    static std::shared_ptr<prometheus::Registry> registry() {
      static auto registry = std::make_shared<prometheus::Registry>();
      return registry;
    }

   public:
    // Handler has access to internal prometheus registry and gathers metrics,
    // prepares them for sending by http
    void setHandler(Handler &handler) override;

    void registerCounterFamily(
        const std::string &name,
        const std::string &help,
        const std::map<std::string, std::string> &labels) override;

    void registerGaugeFamily(
        const std::string &name,
        const std::string &help,
        const std::map<std::string, std::string> &labels) override;

    void registerHistogramFamily(
        const std::string &name,
        const std::string &help,
        const std::map<std::string, std::string> &labels) override;

    void registerSummaryFamily(
        const std::string &name,
        const std::string &help,
        const std::map<std::string, std::string> &labels) override;

    Counter *registerCounterMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels) override;

    Gauge *registerGaugeMetric(
        const std::string &name,
        const std::map<std::string, std::string> &labels) override;

    Histogram *registerHistogramMetric(
        const std::string &name,
        const std::vector<double> &bucket_boundaries,
        const std::map<std::string, std::string> &labels) override;

    Summary *registerSummaryMetric(
        const std::string &name,
        const std::vector<std::pair<double, double>> &quantiles,
        std::chrono::milliseconds max_age,
        int age_buckets,
        const std::map<std::string, std::string> &labels) override;

    // it is used for test purposes
    template <typename T>
    static typename MetricInfo<T>::type *internalMetric(T *metric) {
      return &dynamic_cast<typename MetricInfo<T>::dtype *>(metric)->m_;
    }
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_IMPL_PROMETHEUS_REGISTRY_IMPL_HPP
