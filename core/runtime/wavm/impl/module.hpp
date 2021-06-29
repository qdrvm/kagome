/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP

#include <memory>

#include "log/logger.hpp"

namespace WAVM::Runtime {
  struct Compartment;
  struct Module;
  struct Object;
  using ImportBindings = std::vector<Object *>;
}  // namespace WAVM::Runtime

namespace kagome::runtime::wavm {

  class ModuleInstance;
  class IntrinsicResolver;
  class CompartmentWrapper;

  class Module final {
   public:
    static std::unique_ptr<Module> compileFrom(
        std::shared_ptr<CompartmentWrapper> compartment, gsl::span<const uint8_t> code);

    ~Module() {}

    std::unique_ptr<ModuleInstance> instantiate(IntrinsicResolver &resolver);

   private:
    Module(std::shared_ptr<CompartmentWrapper> compartment,
           std::shared_ptr<WAVM::Runtime::Module> module);

    WAVM::Runtime::ImportBindings link(IntrinsicResolver &resolver);

    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<WAVM::Runtime::Module> module_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
