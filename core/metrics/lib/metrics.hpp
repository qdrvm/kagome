/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_LIB_METRICS_HPP
#define KAGOME_CORE_METRICS_LIB_METRICS_HPP

#include <vector>

namespace kagome::metrics::lib {
  class Counter {
   public:
    virtual void inc() = 0;
    virtual void inc(double val) = 0;
    virtual double val() = 0;
  };

  class Gauge {
   public:
    virtual void inc() = 0;
    virtual void inc(double val) = 0;
    virtual void dec() = 0;
    virtual void dec(double val) = 0;
    virtual void set(double val) = 0;
  };

  class Summary {
   public:
    virtual void observe(const double value) = 0;
  };

  class Histogram {
   public:
    virtual void observe(const double value) = 0;
  };
}  // namespace kagome::metrics::lib

#endif  // KAGOME_CORE_METRICS_LIB_METRICS_HPP
