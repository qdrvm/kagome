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
      Entry(std::shared_ptr<runtime::ModuleInstance> instance,
            uint64_t last_used)
          : instance{instance}, last_used{last_used} {}

      SafeInstance instance;
      uint64_t last_used = 0;
    };
    void cleanup(const std::unique_lock<std::mutex> &lock);

    std::shared_ptr<runtime::ModuleFactory> module_factory_;
    std::mutex instance_cache_mutex_;
    std::unordered_map<ParachainId, Entry> instance_cache_;
    std::map<uint64_t, ParachainId> last_usage_time_;
    const uint32_t instances_limit_ = 43;
    std::atomic<uint64_t> time_ = 0;

    void erase(decltype(instance_cache_)::iterator it);
  };

}  // namespace kagome::parachain