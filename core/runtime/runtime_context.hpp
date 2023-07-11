/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/module_instance.hpp"

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::runtime {
  class ModuleFactory;

  class RuntimeContext {
   public:
    enum class Error {
      ABSENT_HEAP_BASE = 1,
      HEAP_BASE_TOO_LOW,
    };

    RuntimeContext() = delete;
    RuntimeContext(const RuntimeContext &) = delete;
    RuntimeContext &operator=(const RuntimeContext &) = delete;

    RuntimeContext(RuntimeContext &&) = default;
    RuntimeContext &operator=(RuntimeContext &&) = default;

    static outcome::result<RuntimeContext> fromCode(
        const runtime::ModuleFactory &module_factory,
        common::BufferView code_zstd);

    static outcome::result<RuntimeContext> persistent(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt);

    static outcome::result<RuntimeContext> ephemeral(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state);

    const std::shared_ptr<ModuleInstance> module_instance;

   private:
    RuntimeContext(std::shared_ptr<ModuleInstance> module_instance);
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeContext::Error);

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
