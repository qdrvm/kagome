/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime {
  class ParachainHost;
  class RuntimeInstancesPool;
}  // namespace kagome::runtime

namespace kagome::parachain {
  struct ParachainCore;

  class ModulePrecompiler
      : public std::enable_shared_from_this<ModulePrecompiler> {
   public:
    struct Config {
      unsigned precompile_threads_num;
    };

    ModulePrecompiler(
        const Config &config,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<runtime::RuntimeInstancesPool> runtime_cache,
        std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<void> precompileModulesAt(
        const primitives::BlockHash &last_finalized);

    size_t getThreadsNum() const {
      return config_.precompile_threads_num;
    }

   private:
    struct PrecompilationStats;

    outcome::result<void> precompileModulesForCore(
        PrecompilationStats &stats,
        const primitives::BlockHash &last_finalized,
        const ParachainCore &core);

    Config config_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<runtime::RuntimeInstancesPool> runtime_cache_;
    std::shared_ptr<crypto::Hasher> hasher_;

    log::Logger log_ = log::createLogger("ModulePrecompiler", "pvf_executor");
  };
}  // namespace kagome::parachain
