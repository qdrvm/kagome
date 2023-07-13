/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/pvf/pvf_runtime_cache.hpp"

#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"

namespace kagome::parachain {

  outcome::result<PvfRuntimeCache::SafeInstanceRef>
  PvfRuntimeCache::requestInstance(ParachainId para_id,
                                   const common::Hash256 &code_hash,
                                   const ParachainRuntime &code_zstd) {
    std::shared_lock read_lock{instance_cache_mutex_};
    auto it = instance_cache_.find(para_id);
    read_lock.unlock();

    bool it_found = it != instance_cache_.end();

    bool same_hash =
        it_found && it->second.sharedAccess([](const auto &instance) {
          return instance->getCodeHash();
        }) == code_hash;

    if (!(it_found && same_hash)) {
      ParachainRuntime code;
      OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
      OUTCOME_TRY(runtime_module, module_factory_->make(code));
      OUTCOME_TRY(instance, runtime_module->instantiate());
      std::unique_lock write_lock{instance_cache_mutex_};
      if (it_found) {
        erase(it);
      }
      auto [new_it, inserted] = instance_cache_.emplace(para_id, instance);
      BOOST_ASSERT(inserted);
      it = new_it;
    }
    BOOST_ASSERT(it != instance_cache_.end());
    return it->second;
  }

  void PvfRuntimeCache::invalidateExcept(
      std::unordered_set<ParachainId> active_parachains) {
    std::unique_lock write_lock{instance_cache_mutex_};

    for (auto it = instance_cache_.begin(); it != instance_cache_.end(); it++) {
      if (active_parachains.find(it->first) == active_parachains.end()) {
        erase(it);
      }
    }
  }

  void PvfRuntimeCache::erase(decltype(instance_cache_)::iterator it) {
    // instance can be used at this point, so we wait for exclusive access
    // and then extract it from the map (we cannot erase it from inside
    // the exclusive access callback because destroying a locked mutex is
    // UB)
    it->second.exclusiveAccess(
        [this, it](auto) { return instance_cache_.extract(it); });
  }

}  // namespace kagome::parachain