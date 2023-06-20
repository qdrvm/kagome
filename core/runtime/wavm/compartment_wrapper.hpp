/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>

namespace WAVM::Runtime {
  struct Compartment;
}

namespace kagome::runtime::wavm {

  struct CompartmentWrapperImpl;

  /**
   * A wrapper of the WAVM Compartment type, which is used by its garbage
   * collection system
   */
  class CompartmentWrapper final {
   public:
    ~CompartmentWrapper();

    WAVM::Runtime::Compartment *getThreadCompartment();

   private:
    std::mutex m_;
    std::unordered_map<std::thread::id,
                               std::shared_ptr<CompartmentWrapperImpl>>
        impls_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_COMPARTMENT_WRAPPER_HPP
