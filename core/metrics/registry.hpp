/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_REGISTRY_HPP
#define KAGOME_CORE_METRICS_REGISTRY_HPP

#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include <forward_list>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include "metrics/lib/impl/prometheus/metrics_impl.hpp"

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
      using dtype = lib::PrometheusCounter;
      static const unsigned index = 0;
    };

    template <>
    struct MetricInfo<lib::Counter> {
      using type = prometheus::Counter;
      using dtype = lib::PrometheusCounter;
      static const unsigned index = 0;
    };

    template <>
    struct MetricInfo<lib::Gauge> {
      using type = prometheus::Gauge;
      using dtype = lib::PrometheusGauge;
      static const unsigned index = 1;
    };

    template <>
    struct MetricInfo<lib::Histogram> {
      using type = prometheus::Histogram;
      using dtype = lib::PrometheusHistogram;
      static const unsigned index = 2;
    };

    template <>
    struct MetricInfo<lib::Summary> {
      using type = prometheus::Summary;
      using dtype = lib::PrometheusSummary;
      static const unsigned index = 3;
    };

  }  // namespace

  class Registry {
    // prometheus owns families, returns reference
    // sort of cache, not to search every time in prometheus vector of families
    std::unordered_map<std::string,
                       std::reference_wrapper<prometheus::Collectable>>
        family_;
    // main storage of metrics
    std::tuple<std::forward_list<lib::PrometheusCounter>,
               std::forward_list<lib::PrometheusGauge>,
               std::forward_list<lib::PrometheusHistogram>,
               std::forward_list<lib::PrometheusSummary>>
        metrics_;

   public:
    friend class HandlerImpl;
    // Handler has access to internal prometheus registry and gathers metrics,
    // prepares them for sending by http
    void setHandler(Handler *handler);

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

   private:
    static std::shared_ptr<prometheus::Registry> registry() {
      static auto registry = std::make_shared<prometheus::Registry>();
      return registry;
    }
  };
}  // namespace kagome::metrics
#endif  // KAGOME_CORE_METRICS_REGISTRY_HPP
