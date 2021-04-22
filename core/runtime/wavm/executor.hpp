/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
#define KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP

#include "common/buffer.hpp"
#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime::wavm {

  class Executor final {
   public:
    using Buffer = common::Buffer;

    explicit Executor(std::shared_ptr<Memory> memory,
                      std::shared_ptr<ModuleRepository> module_repo)
        : memory_{std::move(memory)}, module_repo_{std::move(module_repo)} {}

    void setState(storage::trie::RootHash new_state) {
      current_state_ = std::move(new_state);
    }

    template <typename Result, typename... Args>
    outcome::result<Result> call(std::string_view name, Args &&...args) {
      OUTCOME_TRY(instance, module_repo_->getInstanceAt(current_state_));

      Buffer encoded_args {};
      if constexpr (sizeof...(args) > 0) {
        OUTCOME_TRY(res, scale::encode(args...));
        encoded_args.put(std::move(res));
      }

      auto res = callInternal(*instance, name, std::move(encoded_args));
      if constexpr (std::is_void_v<Result>) {
        return outcome::success();
      } else {
        return scale::decode<Result>(res);
      }
    }

   private:
    gsl::span<const uint8_t> callInternal(ModuleInstance &instance,
                                          std::string_view name,
                                          Buffer &&args) {
      WasmResult addr{memory_->storeBuffer(args)};
      auto res_span = instance.callExportFunction(name, addr);
      return memory_->loadN(res_span.address, res_span.length);
    }

    storage::trie::RootHash current_state_;
    std::shared_ptr<Memory> memory_;
    // TODO(Harrm): cyclic dependency here
    std::shared_ptr<ModuleRepository> module_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_EXECUTOR_HPP
