/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <shared_mutex>
#include <thread>
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

    outcome::result<SafeInstanceRef> requestInstance(
        ParachainId para_id,
        const common::Hash256 &code_hash,
        const ParachainRuntime &code_zstd);

    void invalidateExcept(std::unordered_set<ParachainId> active_parachains);

   private:
    std::shared_ptr<runtime::ModuleFactory> module_factory_;
    std::shared_mutex instance_cache_mutex_;
    std::map<ParachainId, SafeInstance> instance_cache_;

    void erase(decltype(instance_cache_)::iterator it);
  };

}  // namespace kagome::parachain