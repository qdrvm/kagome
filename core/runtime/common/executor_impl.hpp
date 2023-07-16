/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP

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
          logger_{log::createLogger("Executor", "runtime")} {
      BOOST_ASSERT(module_repo_);
      BOOST_ASSERT(header_repo_);
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    outcome::result<std::unique_ptr<RuntimeContext>> getPersistentContextAt(
        const primitives::BlockHash &block_hash,
        TrieChangesTrackerOpt changes_tracker) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));

      OUTCOME_TRY(ctx,
                  RuntimeContext::persistent(
                      instance, header.state_root, changes_tracker));

      return std::make_unique<RuntimeContext>(std::move(ctx));
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done with the runtime code from \param block_info state
     * on \param storage_state storage state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockInfo &block_info,
                                   const storage::trie::RootHash &storage_state,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_info.hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt(block_info, header.state_root));

      OUTCOME_TRY(ctx, RuntimeContext::ephemeral(instance, storage_state));
      return callWithCache<Result>(ctx, name, std::forward<Args>(args)...);
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the \param block_hash state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAt(const primitives::BlockHash &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));

      OUTCOME_TRY(ctx, RuntimeContext::ephemeral(instance, header.state_root));
      return callWithCache<Result>(ctx, name, std::forward<Args>(args)...);
    }

    /**
     * Call a runtime method in an ephemeral environment, e.g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the genesis state
     */
    outcome::result<common::Buffer> callAtGenesis(
        std::string_view name, const common::Buffer &encoded_args) {
      OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
      OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(genesis_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({genesis_hash, 0},
                                              genesis_header.state_root));

      OUTCOME_TRY(
          ctx, RuntimeContext::ephemeral(instance, genesis_header.state_root));

      return callWithCache<Result>(ctx, name, std::forward<Args>(args)...);
    }

    outcome::result<common::Buffer> callAt(
        const primitives::BlockHash &block_hash,
        std::string_view name,
        const common::Buffer &encoded_args) override {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt({block_hash, header.number},
                                              header.state_root));
      OUTCOME_TRY(ctx, RuntimeContext::ephemeral(instance, header.state_root));
      return callWithCtx(ctx, name, encoded_args);
    }

   private:
    // returns cached results for some common runtime calls
    template <typename Result, typename... Args>
    inline outcome::result<Result> callWithCache(RuntimeContext &ctx,
                                                 std::string_view name,
                                                 Args &&...args) {
      if constexpr (std::is_same_v<Result, primitives::Version>) {
        if (likely(name == "Core_version")) {
          return cache_->getVersion(ctx.module_instance->getCodeHash(), [&] {
            return callWithCtx<Result>(ctx, name, std::forward<Args>(args)...);
          });
        }
      }

      if constexpr (std::is_same_v<Result, primitives::OpaqueMetadata>) {
        if (likely(name == "Metadata_metadata")) {
          return cache_->getMetadata(ctx.module_instance->getCodeHash(), [&] {
            return callWithCtx<Result>(ctx, name, std::forward<Args>(args)...);
          });
        }
      }

      return callWithCtx<Result>(ctx, name, std::forward<Args>(args)...);
    }

    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<RuntimePropertiesCache> cache_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#undef likely

#endif  // KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
