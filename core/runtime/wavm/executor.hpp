/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/impl/crutch.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime::wavm {

  class Executor final {
   public:
    enum class CallPersistency { PERSISTENT, TRANSIENT };

    using Buffer = common::Buffer;

    Executor(std::shared_ptr<TrieStorageProvider> storage_provider,
             std::shared_ptr<Memory> memory,
             std::shared_ptr<ModuleRepository> module_repo,
             std::shared_ptr<host_api::HostApi> host_api)
        : host_api_{std::move(host_api)},
          storage_provider_{std::move(storage_provider)},
          memory_{std::move(memory)},
          module_repo_{std::move(module_repo)} {
      BOOST_ASSERT(memory_);
      BOOST_ASSERT(module_repo_);
      BOOST_ASSERT(storage_provider_);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAtLatest(
        std::string_view name,
        Args &&...args) {
      return callInternal<Result>(storage_provider_->getLatestRoot(),
                                  CallPersistency::PERSISTENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAt(
        storage::trie::RootHash const &state,
        std::string_view name,
        Args &&...args) {
      return callInternal<Result>(state,
                                  CallPersistency::PERSISTENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAtLatest(std::string_view name,
                                         Args &&...args) {
      return callInternal<Result>(storage_provider_->getLatestRoot(),
                                  CallPersistency::TRANSIENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(storage::trie::RootHash const &state,
                                   std::string_view name,
                                   Args &&...args) {
      return callInternal<Result>(
          state, CallPersistency::TRANSIENT, name, std::forward<Args>(args)...);
    }

   private:
    template <typename Result, typename... Args>
    outcome::result<Result> callInternal(storage::trie::RootHash const &state,
                                         CallPersistency persistency,
                                         std::string_view name,
                                         Args &&...args) {
      switch (persistency) {
        case CallPersistency::PERSISTENT: {
          OUTCOME_TRY(storage_provider_->setToPersistentAt(state));
          break;
        }
        case CallPersistency::TRANSIENT: {
          OUTCOME_TRY(storage_provider_->setToEphemeralAt(state));
          break;
        }
      }

      OUTCOME_TRY(instance, module_repo_->getInstanceAt(state));

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }

      gsl::finally([this](){
        host_api_->reset();
        memory_->reset();
      });

      WasmResult addr{memory_->storeBuffer(encoded_args)};

      [[maybe_unused]] auto res_span = instance->callExportFunction(name, addr);

      if(auto batch = storage_provider_->tryGetPersistentBatch(); batch.has_value()) {
        OUTCOME_TRY(batch.value()->commit());
      }
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        auto result = memory_->loadN(res_span.address, res_span.length);
        return scale::decode<Result>(result);
      }
    }

    std::shared_ptr<host_api::HostApi> host_api_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<Memory> memory_;
    // TODO(Harrm): cyclic dependency here
    std::shared_ptr<ModuleRepository> module_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
