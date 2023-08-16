/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "common/blob.hpp"
#include "utils/lru.hpp"
#include "parachain/types.hpp"
#include "utils/safe_object.hpp"

namespace kagome::runtime {
  class ModuleInstance;
  class ModuleFactory;
}  // namespace kagome::runtime

namespace kagome::parachain {

  class PvfRuntimeCache {
   public:
    using SafeInstance = SafeObject<std::shared_ptr<runtime::ModuleInstance>>;
    using SafeInstanceRef = std::reference_wrapper<SafeInstance>;

    PvfRuntimeCache(std::shared_ptr<runtime::ModuleFactory> module_factory_,
                    uint32_t instances_limit);

    outcome::result<SafeInstanceRef> requestInstance(
        ParachainId para_id,
        const common::Hash256 &code_hash,
        const ParachainRuntime &code_zstd);

   private:
    struct Entry {
      Entry(std::shared_ptr<runtime::ModuleInstance> instance)
          : instance{instance} {}

      SafeInstance instance;
    };

    std::shared_ptr<runtime::ModuleFactory> module_factory_;
    std::mutex instance_cache_mutex_;
    Lru<ParachainId, Entry> instance_cache_;

    const uint32_t instances_limit_ = 43;
  };

}  // namespace kagome::parachain