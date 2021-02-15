/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER
#define KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER

#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {

  class ConstWasmProvider : public WasmProvider {
   public:
    explicit ConstWasmProvider(common::Buffer code);

    const common::Buffer &getStateCodeAt(
        const primitives::BlockHash &at) const override;

   private:
    common::Buffer code_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER
