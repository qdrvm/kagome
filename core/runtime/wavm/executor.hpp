/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP

#include <WAVM/RuntimeABI/RuntimeABI.h>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/impl/crutch.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/impl/module_instance.hpp"
#include "runtime/wavm/impl/wavm_memory_provider.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime::wavm {
  class WavmMemoryProvider;
}

namespace kagome::runtime::wavm {

  class Executor final {
   public:
    using Buffer = common::Buffer;

    Executor(std::shared_ptr<TrieStorageProvider> storage_provider,
             std::shared_ptr<MemoryProvider> memory_provider,
             std::shared_ptr<ModuleRepository> module_repo,
             std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
             std::shared_ptr<runtime::RuntimeCodeProvider> code_provider)
        : memory_provider_{std::move(memory_provider)},
          storage_provider_{std::move(storage_provider)},
          code_provider_{std::move(code_provider)},
          module_repo_{std::move(module_repo)},
          header_repo_{std::move(header_repo)},
          logger_{log::createLogger("Executor", "runtime")} {
      BOOST_ASSERT(module_repo_);
      BOOST_ASSERT(code_provider_);
      BOOST_ASSERT(storage_provider_);
      BOOST_ASSERT(memory_provider_);
      BOOST_ASSERT(header_repo_);
    }

    // should be done before any calls
    void setHostApi(std::shared_ptr<host_api::HostApi> host_api) {
      host_api_ = std::move(host_api);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> nestedCall(std::string_view name, Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAt(code_provider_, {}));
      return callInternal<Result>(*instance, name, std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCall(std::string_view name,
                                           Args &&...args) {
      OUTCOME_TRY(storage_provider_->setToPersistentAt(current_state_root_));
      auto res = callInternal<Result>(
          *current_instance_, name, std::forward<Args>(args)...);
      BOOST_ASSERT(storage_provider_->isCurrentlyPersistent());
      if (res.has_value()) {
        OUTCOME_TRY(
            storage_provider_->tryGetPersistentBatch().value()->commit());
      }
      return res;
    }

    outcome::result<void> startNewEnvironment(
        primitives::BlockInfo const &block) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAt(code_provider_, block));
      current_instance_ = std::move(instance);
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
      current_state_root_ = std::move(block_header.state_root);
      return outcome::success();
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAtLatest(std::string_view name,
                                         Args &&...args) {
      if (current_instance_ == nullptr) {
        OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
        OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(0));
        OUTCOME_TRY(
            instance,
            module_repo_->getInstanceAt(code_provider_, {0, genesis_hash}));
        current_instance_ = instance;
        current_state_root_ = genesis_header.state_root;
      }
      OUTCOME_TRY(storage_provider_->setToEphemeralAt(
          storage_provider_->getLatestRoot()));
      return callInternal<Result>(
          *current_instance_, name, std::forward<Args>(args)...);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> callAt(primitives::BlockHash const &block_hash,
                                   std::string_view name,
                                   Args &&...args) {
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt(code_provider_,
                                              {header.number, block_hash}));
      OUTCOME_TRY(storage_provider_->setToEphemeralAt(header.state_root));
      return callInternal<Result>(*instance, name, std::forward<Args>(args)...);
    }

   private:
    template <typename Result, typename... Args>
    outcome::result<Result> callInternal(ModuleInstance &instance,
                                         std::string_view name,
                                         Args &&...args) {
      auto heap_base = instance.getGlobal("__heap_base");
      BOOST_ASSERT(heap_base.has_value()
                   && heap_base.value().type == WAVM::IR::ValueType::i32);

      memory_provider_->resetMemory(heap_base.value().i32);
      auto &memory = memory_provider_->getCurrentMemory().value();

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }
      BOOST_ASSERT(host_api_);
      gsl::finally([this]() { host_api_->reset(); });

      PtrSize addr{memory.storeBuffer(encoded_args)};

      [[maybe_unused]] PtrSize result{0};

      WAVM::Runtime::catchRuntimeExceptions(
          [&result, &instance, &name, &addr] {
            result = instance.callExportFunction(name, addr);
          },
          [this](WAVM::Runtime::Exception *e) {
            logger_->error(WAVM::Runtime::describeException(e));
            WAVM::Runtime::destroyException(e);
          });
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        return scale::decode<Result>(memory.loadN(result.ptr, result.size));
      }
    }

    std::shared_ptr<ModuleInstance> current_instance_;
    storage::trie::RootHash current_state_root_;

    std::shared_ptr<host_api::HostApi> host_api_;
    std::shared_ptr<MemoryProvider> memory_provider_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<runtime::RuntimeCodeProvider> code_provider_;
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
