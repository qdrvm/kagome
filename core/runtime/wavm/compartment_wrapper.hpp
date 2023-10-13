/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>

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
    CompartmentWrapper(std::string &&name);
    ~CompartmentWrapper();

    WAVM::Runtime::Compartment *getCompartment() const;

   private:
    std::shared_ptr<CompartmentWrapperImpl> impl_;
  };

}  // namespace kagome::runtime::wavm
