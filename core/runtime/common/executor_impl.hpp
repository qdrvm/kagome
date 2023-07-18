/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_IMPL_HPP

#include "runtime/executor.hpp"

#include <optional>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/version.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/metadata.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_properties_cache.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"

#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define likely(x) __builtin_expect((x), 1)
#endif
#endif
#ifndef likely
#define likely(x) (x)
#endif

namespace kagome::runtime {

  /**
   * The Runtime executor
   * Provides access to the Runtime API methods, which can be called by their
   * names with the required environment
   */
  class ExecutorImpl : public Executor {
   public:
    using Buffer = common::Buffer;

    ExecutorImpl(
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<RuntimePropertiesCache> cache)
        : module_repo_(std::move(module_repo)),
          header_repo_(std::move(header_repo)),
          cache_(std::move(cache)),
          logger_{log::createLogger("Executor", "runtime")} {}

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    outcome::result<std::unique_ptr<RuntimeContext>> getPersistentContextAt(
        const primitives::BlockHash &block_hash,
        TrieChangesTrackerOpt changes_tracker) override {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));

      OUTCOME_TRY(ctx,
                  RuntimeContext::persistent(
                      instance, header.state_root, changes_tracker));

      return std::make_unique<RuntimeContext>(std::move(ctx));
    }

    outcome::result<std::unique_ptr<RuntimeContext>> getEphemeralContextAt(
        const primitives::BlockHash &block_hash) override {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));

      OUTCOME_TRY(ctx, RuntimeContext::ephemeral(instance, header.state_root));

      return std::make_unique<RuntimeContext>(std::move(ctx));
    }

    outcome::result<std::unique_ptr<RuntimeContext>> getEphemeralContextAt(
        const primitives::BlockHash &block_hash,
        const storage::trie::RootHash &state_hash) override {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));

      OUTCOME_TRY(ctx, RuntimeContext::ephemeral(instance, state_hash));

      return std::make_unique<RuntimeContext>(std::move(ctx));
    }

    outcome::result<std::unique_ptr<RuntimeContext>>
    getEphemeralContextAtGenesis() override {
      OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
      OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(genesis_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({genesis_hash, 0},
                                              genesis_header.state_root));

      OUTCOME_TRY(
          ctx, RuntimeContext::ephemeral(instance, genesis_header.state_root));

      return std::make_unique<RuntimeContext>(std::move(ctx));
    }

    outcome::result<Buffer> callWithCtx(RuntimeContext &ctx,
                                        std::string_view name,
                                        const Buffer &encoded_args) override {
      KAGOME_PROFILE_START(call_execution)
      OUTCOME_TRY(result,
                  ctx.module_instance->callExportFunction(name, encoded_args));
      auto memory = ctx.module_instance->getEnvironment()
                        .memory_provider->getCurrentMemory();
      BOOST_ASSERT(memory.has_value());
      OUTCOME_TRY(ctx.module_instance->resetEnvironment());
      return memory->get().loadN(result.ptr, result.size);
    }

   private:
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<RuntimePropertiesCache> cache_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#undef likely

#endif  // KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
