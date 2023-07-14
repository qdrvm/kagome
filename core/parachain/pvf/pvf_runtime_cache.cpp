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

  PvfRuntimeCache::PvfRuntimeCache(
      std::shared_ptr<runtime::ModuleFactory> module_factory,
      uint32_t instances_limit)
      : module_factory_{module_factory}, instances_limit_{instances_limit} {
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(instances_limit_ > 0);
  }

  outcome::result<PvfRuntimeCache::SafeInstanceRef>
  PvfRuntimeCache::requestInstance(ParachainId para_id,
                                   const common::Hash256 &code_hash,
                                   const ParachainRuntime &code_zstd) {
    ++time_;
    std::unique_lock lock{instance_cache_mutex_};
    auto it = instance_cache_.find(para_id);

    bool it_found = it != instance_cache_.end();

    bool same_hash =
        it_found && it->second.instance.sharedAccess([](const auto &instance) {
          return instance->getCodeHash();
        }) == code_hash;

    if (!(it_found && same_hash)) {
      ParachainRuntime code;
      OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
      OUTCOME_TRY(runtime_module, module_factory_->make(code));
      OUTCOME_TRY(instance, runtime_module->instantiate());
      if (it_found) {
        erase(it);
      }
      SafeObject safe_instance{instance};

      auto [new_it, inserted] =
          instance_cache_.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(para_id),
                                  std::forward_as_tuple(instance, time_));
      BOOST_ASSERT(inserted);
      last_usage_time_.emplace(time_, para_id);
      it = new_it;
      if (instance_cache_.size() > instances_limit_) {
        cleanup(lock);
      }
    } else {
      auto lru_it = last_usage_time_.find(it->second.last_used);
      BOOST_ASSERT(lru_it != last_usage_time_.end());
      last_usage_time_.erase(lru_it);
      last_usage_time_.emplace(time_, para_id);
      it->second.last_used = time_;
    }
    BOOST_ASSERT(it != instance_cache_.end());
    return it->second.instance;
  }

  void PvfRuntimeCache::cleanup(const std::unique_lock<std::mutex> &lock) {
    BOOST_ASSERT(lock.owns_lock());
    BOOST_ASSERT(lock.mutex() == &instance_cache_mutex_);

    for (auto it = last_usage_time_.begin();
         it != last_usage_time_.end()
         && last_usage_time_.size() > instances_limit_;) {
      it = last_usage_time_.erase(it);
    }
  }

  void PvfRuntimeCache::erase(decltype(instance_cache_)::iterator it) {
    // instance can be used at this point, so we wait for exclusive access
    // and then extract it from the map (we cannot erase it from inside
    // the exclusive access callback because destroying a locked mutex is
    // UB)
    it->second.instance.exclusiveAccess(
        [this, it](auto) { return instance_cache_.extract(it); });
  }

}  // namespace kagome::parachain