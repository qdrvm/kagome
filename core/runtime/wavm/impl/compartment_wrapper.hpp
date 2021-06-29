/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP

#include <memory>
#include <string>

namespace WAVM::Runtime {
  struct Compartment;
}

namespace kagome::runtime::wavm {

  struct CompartmentWrapperImpl;

  class CompartmentWrapper final {

   public:
    CompartmentWrapper(std::string&& name);

    WAVM::Runtime::Compartment *getCompartment() const;

   private:
    std::shared_ptr<CompartmentWrapperImpl> impl_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP
