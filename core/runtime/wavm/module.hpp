/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP

#include "runtime/module.hpp"

#include <memory>

#include "log/logger.hpp"

namespace WAVM::Runtime {
  struct Compartment;
  struct Module;
  struct Object;
  using ImportBindings = std::vector<Object *>;
}  // namespace WAVM::Runtime

namespace kagome::runtime::wavm {

  class IntrinsicResolver;
  class CompartmentWrapper;

  class ModuleImpl final : public runtime::Module {
   public:
    static std::unique_ptr<ModuleImpl> compileFrom(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<IntrinsicResolver> resolver,
        gsl::span<const uint8_t> code);

    outcome::result<std::unique_ptr<runtime::ModuleInstance>> instantiate() const override;

   private:
    ModuleImpl(std::shared_ptr<CompartmentWrapper> compartment,
           std::shared_ptr<IntrinsicResolver> resolver,
           std::shared_ptr<WAVM::Runtime::Module> module);

    WAVM::Runtime::ImportBindings link(IntrinsicResolver &resolver) const;

    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<WAVM::Runtime::Module> module_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
