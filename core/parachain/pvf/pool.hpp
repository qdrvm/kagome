/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

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

   private:
    std::shared_ptr<runtime::RuntimeInstancesPoolImpl> pool_;
  };
}  // namespace kagome::parachain
