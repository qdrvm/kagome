/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "primitives/version.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wavm/impl/intrinsic_functions.hpp"
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

    enum class Error { EXECUTION_ERROR = 1 };

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
    outcome::result<Result> persistentCallAtLatest(std::string_view name,
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

    template <typename Result, typename... Args>
    outcome::result<Result> persistentCallAt(
        primitives::BlockInfo const &block_info,
        std::string_view name,
        Args &&...args) {
      OUTCOME_TRY(instance,
                  module_repo_->getInstanceAt(code_provider_, block_info));
      current_instance_ = std::move(instance);
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block_info.hash));
      current_state_root_ = std::move(block_header.state_root);

      return persistentCallAtLatest<Result>(name, std::forward<Args>(args)...);
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
      auto heap_base = instance.getGlobal("__heap_base")
                           .value_or(WAVM::IR::Value{
                               static_cast<WAVM::I32>(kDefaultHeapBase)});
      BOOST_ASSERT(heap_base.type == WAVM::IR::ValueType::i32);

      memory_provider_->resetMemory(heap_base.i32);
      auto &memory = memory_provider_->getCurrentMemory().value();

      Buffer encoded_args{};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(std::forward<Args>(args)...));
        encoded_args.put(std::move(res));
      }
      BOOST_ASSERT(host_api_);
      gsl::finally([this]() { host_api_->reset(); });

      PtrSize args_span{memory.storeBuffer(encoded_args)};

      auto result = execute(instance, name, args_span);
      // TODO(Harrm): This is for debug purposes, remove before merging
      if (!result) {
        logger_->critical(result.error().message());
        // std::terminate();
      }

      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        return scale::decode<Result>(
            memory.loadN(result.value().ptr, result.value().size));
      }
    }

    outcome::result<PtrSize> execute(ModuleInstance &instance,
                                     std::string_view name,
                                     PtrSize args);

    std::shared_ptr<ModuleInstance> current_instance_;
    storage::trie::RootHash current_state_root_;

    std::shared_ptr<host_api::HostApi> host_api_;
    std::shared_ptr<MemoryProvider> memory_provider_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider_;
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::wavm, Executor::Error)

#endif  // KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
