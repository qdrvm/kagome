/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/impl/crutch.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/impl/module_instance.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime::wavm {

  class Executor final {
   public:
    enum class CallPersistency { PERSISTENT, TRANSIENT };

    using Buffer = common::Buffer;

    Executor(std::shared_ptr<TrieStorageProvider> storage_provider,
             std::shared_ptr<runtime::Memory> memory,
             std::shared_ptr<ModuleRepository> module_repo,
             std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
        : storage_provider_{std::move(storage_provider)},
          memory_{std::move(memory)},
          module_repo_{std::move(module_repo)},
          header_repo_{std::move(header_repo)} {
      BOOST_ASSERT(memory_);
      BOOST_ASSERT(module_repo_);
      BOOST_ASSERT(storage_provider_);
      BOOST_ASSERT(header_repo_);
    }

    // should be done before any calls
    void setHostApi(std::shared_ptr<host_api::HostApi> host_api) {
      host_api_ = std::move(host_api);
    }

    // should be done before any calls
    void setCodeProvider(
        std::shared_ptr<runtime::RuntimeCodeProvider> code_provider) {
      code_provider_ = std::move(code_provider);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAtLatest(std::string_view name,
                                                   Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAtLatest(code_provider_));
      return callInternal<Result>(storage_provider_->getLatestRoot(),
                                  *instance,
                                  CallPersistency::PERSISTENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAt(primitives::BlockInfo const &block,
                                             std::string_view name,
                                             Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAtLatest(code_provider_));
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
      return callInternal<Result>(block_header.state_root,
                                  *instance,
                                  CallPersistency::PERSISTENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAtLatest(std::string_view name,
                                         Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAtLatest(code_provider_));
      return callInternal<Result>(storage_provider_->getLatestRoot(),
                                  *instance,
                                  CallPersistency::TRANSIENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(primitives::BlockInfo const &block,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAt(code_provider_, block));
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
      return callInternal<Result>(block_header.state_root,
                                  *instance,
                                  CallPersistency::TRANSIENT,
                                  name,
                                  std::forward<Args>(args)...);
    }

   private:
    template <typename Result, typename... Args>
    outcome::result<Result> callInternal(storage::trie::RootHash const &state,
                                         ModuleInstance &instance,
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
      auto heap_base = instance.getGlobal("__heap_base");
      BOOST_ASSERT(heap_base.has_value()
                   && heap_base.value().type == ValueType::i32);
      memory_->setHeapBase(heap_base.value().i32);

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }
      BOOST_ASSERT(host_api_);
      gsl::finally([this]() {
        host_api_->reset();
        memory_->reset();
      });

      WasmResult addr{memory_->storeBuffer(encoded_args)};

      [[maybe_unused]] auto res_span = instance.callExportFunction(name, addr);

      if (auto batch = storage_provider_->tryGetPersistentBatch();
          batch.has_value()) {
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
    std::shared_ptr<runtime::Memory> memory_;
    std::shared_ptr<runtime::RuntimeCodeProvider> code_provider_;
    // TODO(Harrm): cyclic dependency here
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
