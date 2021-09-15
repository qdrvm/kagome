/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/compartment_wrapper.hpp"

#include <WAVM/Runtime/Runtime.h>
#include <boost/assert.hpp>
#include <stdexcept>

namespace kagome::runtime::wavm {

  size_t CompartmentWrapper::Count = 0;

  struct CompartmentWrapperImpl final {
    CompartmentWrapperImpl(
        WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment)
        : compartment_{std::move(compartment)} {}

    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  };

  CompartmentWrapper::CompartmentWrapper(std::string &&name)
      : impl_{std::make_unique<CompartmentWrapperImpl>(WAVM::Runtime::GCPointer{
          WAVM::Runtime::createCompartment(std::move(name))})} {
    Count++;
  }

  CompartmentWrapper::~CompartmentWrapper() {
    BOOST_VERIFY(
        WAVM::Runtime::tryCollectCompartment(std::move(impl_->compartment_)));
    Count--;
  }

  WAVM::Runtime::Compartment *CompartmentWrapper::getCompartment() const {
    return impl_->compartment_;
  }

}  // namespace kagome::runtime::wavm
