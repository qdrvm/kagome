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
#include "runtime/runtime_environment_factory.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::runtime {

  /**
   * The Runtime executor
   * Provides access to the Runtime API methods, which can be called by their
   * names with the required environment
   */
  class Executor {
   public:
    using Buffer = common::Buffer;

    Executor(
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<RuntimeEnvironmentFactory> env_factory)
        : header_repo_{std::move(header_repo)},
          env_factory_{std::move(env_factory)} {
      BOOST_ASSERT(header_repo_ != nullptr);
      BOOST_ASSERT(env_factory_ != nullptr);
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the latest chosen state (the state can be chosen
     * with persistentCallAt)
     */
    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAtLatest(std::string_view name,
                                                   Args &&...args) {
      OUTCOME_TRY(env, env_factory_->start()->persistent().make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        BOOST_ASSERT_MSG(
            env->batch.has_value(),
            "Persistent batch should always exist for persistent call");
        OUTCOME_TRY(env->batch.value()->commit());
      }
      return res;
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAt(
        primitives::BlockInfo const &block_info,
        std::string_view name,
        Args &&...args) {
      OUTCOME_TRY(
          env, env_factory_->start()->persistent().setState(block_info).make());
      auto res = callInternal<Result>(*env, name, std::forward<Args>(args)...);
      if (res) {
        BOOST_ASSERT_MSG(
            env->batch.has_value(),
            "Persistent batch should always exist for persistent call");
        OUTCOME_TRY(env->batch.value()->commit());
      }
      return res;
    }

    /**
     * Call a runtime method in an ephemeral environment, e. g. the storage
     * changes, made by this call, will NOT persist in the node's Trie storage
     * The call will be done on the latest chosen state (the state can be chosen
     * with persistentCallAt)
     */
    template <typename Result, typename... Args>
    outcome::result<Result> callAtLatest(std::string_view name,
                                         Args &&...args) {
      OUTCOME_TRY(env, env_factory_->start()->make());
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
      OUTCOME_TRY(
          env,
          env_factory_->start()->setState({header.number, block_hash}).make());
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
