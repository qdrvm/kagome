/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pool.hpp"

#include "application/app_configuration.hpp"
#include "metrics/histogram_timer.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"

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

  metrics::HistogramHelper metric_code_size{
      "kagome_parachain_candidate_validation_code_size",
      "The size of the decompressed WASM validation blob used for checking a "
      "candidate",
      metrics::exponentialBuckets(16384, 2, 10),
  };

  PvfPool::PvfPool(const application::AppConfiguration &app_config,
                   std::shared_ptr<runtime::ModuleFactory> module_factory,
                   std::shared_ptr<runtime::InstrumentWasm> instrument)
      : pool_{std::make_shared<runtime::RuntimeInstancesPoolImpl>(
          app_config,
          std::move(module_factory),
          std::move(instrument),
          app_config.parachainRuntimeInstanceCacheSize())} {}

  outcome::result<void> PvfPool::precompile(
      const Hash256 &code_hash,
      BufferView code_zstd,
      const runtime::RuntimeContext::ContextParams &config) const {
    auto timer = metric_pvf_preparation_time().timer();
    return pool_->precompile(
        code_hash,
        [&]() mutable -> runtime::RuntimeCodeProvider::Result {
          OUTCOME_TRY(code, runtime::uncompressCodeIfNeeded(code_zstd));
          metric_code_size.observe(code.size());
          return std::make_shared<Buffer>(code);
        },
        config);
  }
}  // namespace kagome::parachain
