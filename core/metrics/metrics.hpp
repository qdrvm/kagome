/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_METRICS_HPP
#define KAGOME_CORE_METRICS_METRICS_HPP

#include <memory>
#include <vector>

#include "metrics/registry.hpp"

namespace kagome::metrics {
  using RegistryPtr = std::unique_ptr<Registry>;

  // the function recommended to use to create a registry of the chosen
  // implementation
  RegistryPtr createRegistry();

  class Counter {
   public:
    virtual ~Counter() = default;
    virtual void inc() = 0;
    virtual void inc(double val) = 0;
  };

  class Gauge {
   public:
    virtual ~Gauge() = default;
    virtual void inc() = 0;
    virtual void inc(double val) = 0;
    virtual void dec() = 0;
    virtual void dec(double val) = 0;
    virtual void set(double val) = 0;
  };

  class Summary {
   public:
    virtual ~Summary() = default;
    virtual void observe(const double value) = 0;
  };

  class Histogram {
   public:
    virtual ~Histogram() = default;
    virtual void observe(const double value) = 0;
  };
}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_METRICS_HPP
