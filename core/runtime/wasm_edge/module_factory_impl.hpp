/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"
#include "crypto/hasher.hpp"

namespace kagome::runtime::wasm_edge {

  class ModuleFactoryImpl : public ModuleFactory {
   public:
    outcome::result<std::shared_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime::wasm_edge
