/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_CORE_FACTORY
#define KAGOME_CORE_RUNTIME_CORE_FACTORY

#include "runtime/core.hpp"

namespace kagome::runtime {

  namespace binaryen {
    class RuntimeEnvironmentFactory;
  }

  /**
   * An abstract factory that enables construction of Core objects over specific
   * WASM code
   */
  class CoreFactory {
   public:
    virtual ~CoreFactory() = default;

    /**
     * Creates a Core API object backed by the code that \arg wasm_provider
     * serves
     */
    virtual std::unique_ptr<Core> createWithCode(
        std::shared_ptr<WasmProvider> wasm_provider) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_CORE_FACTORY
