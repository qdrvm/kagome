/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/compartment_wrapper.hpp"

#include <sstream>
#include <stdexcept>

#include <WAVM/Runtime/Runtime.h>
#include <boost/assert.hpp>

namespace kagome::runtime::wavm {

  struct CompartmentWrapperImpl final {
    CompartmentWrapperImpl(
        WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment)
        : compartment_{std::move(compartment)} {}

    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment_;
  };

  CompartmentWrapper::~CompartmentWrapper() {
    std::terminate();
    // BOOST_VERIFY(
    //     WAVM::Runtime::tryCollectCompartment(std::move(impls_->compartment_)));
  }

  WAVM::Runtime::Compartment *CompartmentWrapper::getThreadCompartment() {
    if (auto it = impls_.find(std::this_thread::get_id()); it != impls_.end()) {
      return it->second->compartment_;
    }
    std::stringstream ss;
    ss << "Compartment on thread" << std::this_thread::get_id();
    std::string name = ss.str();
    WAVM::Runtime::GCPointer ptr{
        WAVM::Runtime::createCompartment(std::move(name))};
    std::scoped_lock lock{m_};
    auto [it, _] = impls_.insert_or_assign(
        std::this_thread::get_id(),
        std::make_shared<CompartmentWrapperImpl>(std::move(ptr)));
    return it->second->compartment_;
  }

}  // namespace kagome::runtime::wavm
