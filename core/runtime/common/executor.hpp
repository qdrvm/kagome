/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP

#include "runtime/raw_executor.hpp"

#include <optional>

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
#include "runtime/runtime_environment_factory.hpp"
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
  class Executor : public RawExecutor {
   public:
    using Buffer = common::Buffer;

    Executor(std::shared_ptr<RuntimeEnvironmentFactory> env_factory,
             std::shared_ptr<RuntimePropertiesCache> cache)
        : env_factory_(std::move(env_factory)),
          cache_(std::move(cache)),
          logger_{log::createLogger("Executor", "runtime")} {}

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done with the runtime code from \param block_info state
     * on \param storage_state storage state
     */
    outcome::result<std::unique_ptr<RuntimeEnvironment>> persistentAt(
        primitives::BlockInfo const &block_info,
        storage::trie::RootHash const &storage_state) {
      OUTCOME_TRY(
          env,
          env_factory_->start(block_info, storage_state)->persistent().make());
      return std::move(env);
    }

    /**
     * Call a runtime method in a persistent environment, e. g. the storage
     * changes, made by this call, will persist in the node's Trie storage
     * The call will be done on the \param block_info state
     */
    outcome::result<std::unique_ptr<RuntimeEnvironment>> persistentAt(
        primitives::BlockHash const &block_hash) {
      OUTCOME_TRY(env_template, env_factory_->start(block_hash));
      OUTCOME_TRY(env, env_template->persistent().make());
      return std::move(env);
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
      return callMediateInternal<Result>(
          *env, name, std::forward<Args>(args)...);
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
      return callMediateInternal<Result>(
          *env, name, std::forward<Args>(args)...);
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
      return callMediateInternal<Result>(
          *env, name, std::forward<Args>(args)...);
    }

    outcome::result<common::Buffer> callAtRaw(
        const primitives::BlockHash &block_hash,
        std::string_view name,
        const common::Buffer &encoded_args) override {
      OUTCOME_TRY(env_template, env_factory_->start(block_hash));
      OUTCOME_TRY(env, env_template->make());

      auto &memory = env->memory_provider->getCurrentMemory()->get();

      KAGOME_PROFILE_START(call_execution)

      auto result_span =
          env->module_instance->callExportFunction(name, encoded_args);

      KAGOME_PROFILE_END(call_execution)
      OUTCOME_TRY(span, result_span);

      OUTCOME_TRY(env->module_instance->resetEnvironment());
      auto result = memory.loadN(span.ptr, span.size);

      return result;
    }

    /**
     * Internal method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    inline outcome::result<Result> callMediateInternal(RuntimeEnvironment &env,
                                                       std::string_view name,
                                                       Args &&...args) {
      if constexpr (std::is_same_v<Result, primitives::Version>) {
        if (likely(name == "Core_version")) {
          return cache_->getVersion(env.module_instance->getCodeHash(), [&] {
            return call<Result>(env, name, std::forward<Args>(args)...);
          });
        }
      }

      if constexpr (std::is_same_v<Result, primitives::OpaqueMetadata>) {
        if (likely(name == "Metadata_metadata")) {
          return cache_->getMetadata(env.module_instance->getCodeHash(), [&] {
            return call<Result>(env, name, std::forward<Args>(args)...);
          });
        }
      }

      return call<Result>(env, name, std::forward<Args>(args)...);
    }

    /**
     * Internal method for calling a Runtime API method
     * Resets the runtime memory with the module's heap base,
     * encodes the arguments with SCALE codec, calls the method from the
     * provided module instance and  returns a result, decoded from SCALE.
     * Changes, made to the Host API state, are reset after the call.
     */
    template <typename Result, typename... Args>
    outcome::result<Result> call(RuntimeEnvironment &env,
                                 std::string_view name,
                                 Args &&...args) {
      auto &memory = env.memory_provider->getCurrentMemory()->get();

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }

      KAGOME_PROFILE_START(call_execution)

      auto result_span =
          env.module_instance->callExportFunction(name, encoded_args);

      KAGOME_PROFILE_END(call_execution)
      OUTCOME_TRY(span, result_span);

      OUTCOME_TRY(env.module_instance->resetEnvironment());

      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        auto result = memory.loadN(span.ptr, span.size);
        Result t{};
        scale::ScaleDecoderStream s(result);
        try {
          s >> t;
          // Check whether the whole byte buffer was consumed
          if (s.hasMore(1)) {
            SL_ERROR(logger_,
                     "Runtime API call result size exceeds the size of the "
                     "type to initialize {}",
                     typeid(Result).name());
            return outcome::failure(std::errc::illegal_byte_sequence);
          }
          return outcome::success(std::move(t));
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      }
    }

   private:
    std::shared_ptr<RuntimeEnvironmentFactory> env_factory_;
    std::shared_ptr<RuntimePropertiesCache> cache_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#undef likely

#endif  // KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
