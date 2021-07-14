/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP

#include "intrinsic_module_instance.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicModule final {
   public:
    static inline const WAVM::IR::MemoryType kIntrinsicMemoryType{
        WAVM::IR::MemoryType(
            false, WAVM::IR::IndexType::i32, {20, UINT64_MAX})};
    static constexpr std::string_view kIntrinsicMemoryName = "Host Memory";

    explicit IntrinsicModule(std::shared_ptr<CompartmentWrapper> compartment)
        : compartment_{compartment} {}

    std::unique_ptr<IntrinsicModuleInstance> instantiate() {
      return std::make_unique<IntrinsicModuleInstance>(
          WAVM::Intrinsics::instantiateModule(compartment_->getCompartment(),
                                              {&module_},
                                              "Intrinsic Module Instance"));
    }

    template <typename Ret, typename... Args>
    void addFunction(std::string_view name, Ret (*f)(Args...)) {
      auto intrinsic = std::make_unique<WAVM::Intrinsics::Function>(
          &module_,
          name.data(),
          (void *)f,
          WAVM::Intrinsics::inferIntrinsicFunctionType(f));
      functions_.insert(std::pair{name, std::move(intrinsic)});
    }

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;

    WAVM::Intrinsics::Module module_;

    // actually used, because added to the intrinsic module
    [[maybe_unused]] WAVM::Intrinsics::Memory memory_{
        &module_, kIntrinsicMemoryName.data(), kIntrinsicMemoryType};

    std::unordered_map<std::string, std::unique_ptr<WAVM::Intrinsics::Function>>
        functions_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP
