/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_CORE_FACTORY
#define KAGOME_CORE_RUNTIME_CORE_FACTORY

#include "runtime/core.hpp"

namespace kagome::runtime {

  class CoreFactory {
   public:
    virtual ~CoreFactory() = default;

    virtual std::unique_ptr<Core> createWithCode(
        std::shared_ptr<WasmProvider> wasm_provider) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_CORE_FACTORY
