/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP

#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"

#include <boost/assert.hpp>
#include <unordered_map>

#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicModule final {
   public:
    static constexpr std::string_view kIntrinsicMemoryName = "Runtime Memory";

    explicit IntrinsicModule(std::shared_ptr<CompartmentWrapper> compartment,
                             std::shared_ptr<ModuleParams> module_params)
        : compartment_{compartment},
          module_params_{std::move(module_params)},
          memory_{&module_,
                  kIntrinsicMemoryName.data(),
                  module_params->intrinsicMemoryType} {}

    IntrinsicModule(IntrinsicModule &module,
                    std::shared_ptr<ModuleParams> module_params)
        : compartment_{module.compartment_},
          module_params_{std::move(module_params)},
          memory_{&module_,
                  kIntrinsicMemoryName.data(),
                  module_params->intrinsicMemoryType} {}

    std::unique_ptr<IntrinsicModuleInstance> instantiate() const {
      BOOST_ASSERT_MSG(
          !functions_.empty(),
          "Host API methods are not registered within IntrinsicModule! See "
          "runtime/wavm/intrinsics/intrinsic_functions.hpp");
      return std::make_unique<IntrinsicModuleInstance>(
          WAVM::Intrinsics::instantiateModule(compartment_->getCompartment(),
                                              {&module_},
                                              "Intrinsic Module Instance"),
          compartment_,
          module_params_);
    }

    template <typename Ret, typename... Args>
    void addFunction(std::string_view name,
                     Ret (*f)(WAVM::Runtime::ContextRuntimeData *, Args...),
                     WAVM::IR::FunctionType type) {
      auto inferred_type = WAVM::Intrinsics::inferIntrinsicFunctionType(f);
      BOOST_ASSERT(type.results() == inferred_type.results());
      BOOST_ASSERT(type.params() == inferred_type.params());
      auto intrinsic = std::make_unique<WAVM::Intrinsics::Function>(
          &module_, name.data(), (void *)f, inferred_type);
      functions_.insert(std::pair{name, std::move(intrinsic)});
    }

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    WAVM::Intrinsics::Module module_;

    // actually used, because added to the intrinsic module
    WAVM::Intrinsics::Memory memory_;

    std::unordered_map<std::string, std::unique_ptr<WAVM::Intrinsics::Function>>
        functions_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_HPP
