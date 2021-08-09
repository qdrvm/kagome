/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_EXECUTOR_HPP

#include <boost/optional.hpp>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
#include "log/logger.hpp"
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
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<RuntimeEnvironmentFactory> env_factory)
        : header_repo_{std::move(header_repo)},
          env_factory_{std::move(env_factory)},
          logger_{log::createLogger("Executor", "runtime")} {
      BOOST_ASSERT(header_repo_ != nullptr);
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
      SL_DEBUG(logger_,
               "Runtime calls {} persistently at state: #{} hash: {} "
               "state: {}",
               name,
               block_info.number,
               block_info.hash.toHex(),
               storage_state.toHex());
      OUTCOME_TRY(
          env,
          env_factory_->start(block_info, storage_state)->persistent().make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        BOOST_ASSERT_MSG(
            env->batch.has_value(),
            "Persistent batch should always exist for persistent call");
        OUTCOME_TRY(state_root, env->batch.value()->commit());
        SL_DEBUG(logger_,
                 "Runtime call committed new state with hash {}",
                 state_root.toHex());
        if constexpr (std::is_void_v<Result>) {
          return PersistentResult<Result>{state_root};
        } else {
          return PersistentResult<Result>{std::move(res.value()), state_root};
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
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      return persistentCallAt<Result>(
          primitives::BlockInfo{header.number, block_hash},
          header.state_root,
          name,
          std::forward<Args>(args)...);
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
      SL_DEBUG(logger_,
               "Runtime calls {} at state: #{} hash: {} state: {}",
               name,
               block_info.number,
               block_info.hash.toHex(),
               storage_state.toHex());
      OUTCOME_TRY(env, env_factory_->start(block_info, storage_state)->make());
      return callInternal<Result>(*env, name, std::forward<Args>(args)...);
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the \param block_hash state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAt(primitives::BlockHash const &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      SL_DEBUG(logger_,
               "Runtime calls {} at #{} hash: {}, state: {}",
               name,
               header.number,
               block_hash.toHex(),
               header.state_root.toHex());
      OUTCOME_TRY(
          env,
          env_factory_->start({header.number, block_hash}, header.state_root)
              ->make());
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
      auto &memory = env.memory;

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }

      PtrSize args_span{memory.storeBuffer(encoded_args)};

      OUTCOME_TRY(result,
                  env.module_instance->callExportFunction(name, args_span));
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        return scale::decode<Result>(memory.loadN(result.ptr, result.size));
      }
    }
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<RuntimeEnvironmentFactory> env_factory_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_EXECUTOR_HPP
