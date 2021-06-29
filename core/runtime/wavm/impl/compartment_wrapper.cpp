/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/compartment_wrapper.hpp"

#include <WAVM/Runtime/Runtime.h>

namespace kagome::runtime::wavm {

  struct CompartmentWrapperImpl final {
    CompartmentWrapperImpl(
        WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment)
        : compartment_{std::move(compartment)} {}

    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  };

  CompartmentWrapper::CompartmentWrapper(std::string &&name)
      : impl_{std::make_unique<CompartmentWrapperImpl>(WAVM::Runtime::GCPointer{
          WAVM::Runtime::createCompartment(std::move(name))})} {}

  WAVM::Runtime::Compartment *CompartmentWrapper::getCompartment() const {
    return impl_->compartment_;
  }

}  // namespace kagome::runtime::wavm
