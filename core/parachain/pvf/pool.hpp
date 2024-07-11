/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "runtime/runtime_context.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::runtime {
  class InstrumentWasm;
  class ModuleFactory;
  class RuntimeInstancesPoolImpl;
}  // namespace kagome::runtime

namespace kagome::parachain {
  /**
   * Reused by `PvfPrecheck` and `PvfImpl` to measure pvf compile time metric.
   */
  class PvfPool {
   public:
    PvfPool(const application::AppConfiguration &app_config,
            std::shared_ptr<runtime::ModuleFactory> module_factory,
            std::shared_ptr<runtime::InstrumentWasm> instrument);

    auto &pool() const {
      return pool_;
    }

    /**
     * Measures `kagome_parachain_candidate_validation_code_size` and
     * `kagome_pvf_preparation_time` metrics.
     */
    outcome::result<void> precompile(
        const Hash256 &code_hash,
        BufferView code_zstd,
        const runtime::RuntimeContext::ContextParams &config) const;

   private:
    std::shared_ptr<runtime::RuntimeInstancesPoolImpl> pool_;
  };
}  // namespace kagome::parachain
