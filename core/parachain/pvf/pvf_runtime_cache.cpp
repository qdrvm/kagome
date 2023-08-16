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
      : module_factory_{module_factory},
        instance_cache_{instances_limit},
        instances_limit_{instances_limit} {
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(instances_limit_ > 0);
  }

  outcome::result<PvfRuntimeCache::SafeInstanceRef>
  PvfRuntimeCache::requestInstance(ParachainId para_id,
                                   const common::Hash256 &code_hash,
                                   const ParachainRuntime &code_zstd) {
    std::unique_lock lock{instance_cache_mutex_};
    auto it = instance_cache_.find(para_id);

    bool it_found = it != instance_cache_.end();

    bool same_hash = false;
    if (it_found) {
      auto& instance = it.value().instance;
      same_hash = instance.sharedAccess([](const auto &instance) {
        return instance->getCodeHash();
      }) == code_hash;
    }
    if (!(it_found && same_hash)) {
      ParachainRuntime code;
      OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
      OUTCOME_TRY(runtime_module, module_factory_->make(code));
      OUTCOME_TRY(instance, runtime_module->instantiate());
      if (it_found) {
        auto& old_instance = it.value().instance;
        // instance can be used at this point, so we wait for exclusive access
        old_instance.exclusiveAccess([this, &para_id](auto &instance) {
          return instance_cache_.extract(para_id);
        });
      }
      SafeObject safe_instance{instance};

      auto [new_it, inserted] =
          instance_cache_.emplace(para_id, std::move(instance));
      BOOST_ASSERT(inserted);
      it = new_it;
    }
    BOOST_ASSERT(it_found);
    auto& instance = it.value().instance;
    return instance;
  }

}  // namespace kagome::parachain