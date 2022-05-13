/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_EXECUTOR_HPP

#include <optional>
#include <fstream>
#include <thread>

#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/version.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/persistent_result.hpp"
#include "runtime/runtime_environment_factory.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {

  /**
   * The Runtime executor
   * Provides access to the Runtime API methods, which can be called by their
   * names with the required environment
   */
  class Executor final {
   public:
    using Buffer = common::Buffer;

    Executor(
        std::shared_ptr<RuntimeEnvironmentFactory> env_factory)
        : env_factory_{std::move(env_factory)},
          logger_{log::createLogger("Executor", "runtime")} {
      BOOST_ASSERT(env_factory_ != nullptr);
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done with the runtime code from \param block_info state
     * on \param storage_state storage state
     */
    template <typename Result, typename... Args>
    outcome::result<PersistentResult<Result>> persistentCallAt(
        primitives::BlockInfo const &block_info,
        storage::trie::RootHash const &storage_state,
        std::string_view name,
        Args &&...args) {
      OUTCOME_TRY(
          env,
          env_factory_->start(block_info, storage_state)->persistent().make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        OUTCOME_TRY(new_state_root, commitState(*env));
        if constexpr (std::is_void_v<Result>) {
          return PersistentResult<Result>{new_state_root};
        } else {
          return PersistentResult<Result>{std::move(res.value()),
                                          new_state_root};
        }
      }
      return res.error();
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the genesis block state
     */
    template <typename Result, typename... Args>
    outcome::result<PersistentResult<Result>> persistentCallAtGenesis(
        std::string_view name, Args &&...args) {
      OUTCOME_TRY(env_template, env_factory_->start());
      OUTCOME_TRY(env, env_template->persistent().make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        OUTCOME_TRY(new_state_root, commitState(*env));
        if constexpr (std::is_void_v<Result>) {
          return PersistentResult<Result>{new_state_root};
        } else {
          return PersistentResult<Result>{std::move(res.value()),
                                          new_state_root};
        }
      }
      return res.error();
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    template <typename Result, typename... Args>
    outcome::result<PersistentResult<Result>> persistentCallAt(
        primitives::BlockHash const &block_hash,
        std::string_view name,
        Args &&...args) {
      OUTCOME_TRY(env_template, env_factory_->start(block_hash));
      OUTCOME_TRY(env, env_template->persistent().make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        OUTCOME_TRY(new_state_root, commitState(*env));
        if constexpr (std::is_void_v<Result>) {
          return PersistentResult<Result>{new_state_root};
        } else {
          return PersistentResult<Result>{std::move(res.value()),
                                          new_state_root};
        }
      }
      return res.error();
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done with the runtime code from \param block_info state
     * on \param storage_state storage state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAt(primitives::BlockInfo const &block_info,
                                   storage::trie::RootHash const &storage_state,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(env, env_factory_->start(block_info, storage_state)->make());
      return callInternal<Result>(*env, name, std::forward<Args>(args)...);
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the \param block_hash state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAt(primitives::BlockHash const &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(env_template, env_factory_->start(block_hash));
      OUTCOME_TRY(env, env_template->make());
      return callInternal<Result>(*env, name, std::forward<Args>(args)...);
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the genesis state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAtGenesis(std::string_view name,
                                          Args &&...args) {
      OUTCOME_TRY(env_template, env_factory_->start());
      OUTCOME_TRY(env, env_template->make());
      return callInternal<Result>(*env, name, std::forward<Args>(args)...);
    }

   private:
    /**
     * Internal method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callInternal(RuntimeEnvironment &env,
                                         std::string_view name,
                                         Args &&...args) {
      auto &memory = env.memory_provider->getCurrentMemory()->get();

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }

      KAGOME_PROFILE_START(call_execution)
      SL_TRACE(logger_, "Executing call {} with memory {} on instance {}", name, memory.debugDescription(), fmt::ptr(env.module_instance.get()));
      auto result_span =
                  env.module_instance->callExportFunction(name, encoded_args);
      KAGOME_PROFILE_END(call_execution)
      OUTCOME_TRY(span, result_span);

      OUTCOME_TRY(env.module_instance->resetEnvironment());

      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        return scale::decode<Result>(memory.loadN(span.ptr, span.size));
      }
    }

    outcome::result<storage::trie::RootHash> commitState(
        const RuntimeEnvironment &env) {
      KAGOME_PROFILE_START(state_commit)
      BOOST_ASSERT_MSG(
          env.storage_provider->tryGetPersistentBatch(),
          "Current batch should always be persistent for a persistent call");
      auto persistent_batch =
          env.storage_provider->tryGetPersistentBatch().value();
      OUTCOME_TRY(new_state_root, persistent_batch->commit());
      SL_DEBUG(logger_,
               "Runtime call committed new state with hash {}",
               new_state_root.toHex());
      KAGOME_PROFILE_END(state_commit)
      return std::move(new_state_root);
    }

    std::shared_ptr<RuntimeEnvironmentFactory> env_factory_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_EXECUTOR_HPP
