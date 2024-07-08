/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pool.hpp"

#include "application/app_configuration.hpp"
#include "metrics/histogram_timer.hpp"
#include "runtime/common/runtime_instances_pool.hpp"

namespace kagome::parachain {
  inline auto &metric_pvf_preparation_time() {
    static metrics::HistogramTimer metric{
        "kagome_pvf_preparation_time",
        "Time spent in preparing PVF artifacts in seconds",
        {
            0.1,
            0.5,
            1.0,
            2.0,
            3.0,
            10.0,
            20.0,
            30.0,
            60.0,
            120.0,
            240.0,
            360.0,
            480.0,
        },
    };
    return metric;
  }

  struct PvfPoolWrapper : runtime::ModuleFactory {
    PvfPoolWrapper(std::shared_ptr<runtime::ModuleFactory> inner)
        : inner_{std::move(inner)} {}

    runtime::CompilationOutcome<std::shared_ptr<runtime::Module>> make(
        common::BufferView code) const override {
      auto timer = metric_pvf_preparation_time().timer();
      return inner_->make(code);
    }

    std::shared_ptr<runtime::ModuleFactory> inner_;
  };

  PvfPool::PvfPool(const application::AppConfiguration &app_config,
                   std::shared_ptr<runtime::ModuleFactory> module_factory,
                   std::shared_ptr<runtime::InstrumentWasm> instrument)
      : pool_{std::make_shared<runtime::RuntimeInstancesPoolImpl>(
          std::make_shared<PvfPoolWrapper>(std::move(module_factory)),
          std::move(instrument),
          app_config.parachainRuntimeInstanceCacheSize())} {}
}  // namespace kagome::parachain
