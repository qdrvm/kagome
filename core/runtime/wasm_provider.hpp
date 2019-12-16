/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP

#include "common/buffer.hpp"

namespace kagome::runtime {
  /**
   * @class WasmProvider keeps and provides wasm state code
   */
  class WasmProvider {
   public:
    virtual ~WasmProvider() = default;

    /**
     * @return wasm runtime code
     */
    virtual const common::Buffer &getStateCode() const = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WASM_PROVIDER_HPP
