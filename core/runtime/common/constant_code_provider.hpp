/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER
#define KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER

#include "runtime/runtime_code_provider.hpp"

namespace kagome::runtime {

  class ConstantCodeProvider final : public RuntimeCodeProvider {
   public:
    explicit ConstantCodeProvider(common::Buffer code);

    outcome::result<CodeAndItsState> getCodeAt(
        const primitives::BlockInfo &at) const override;

    outcome::result<CodeAndItsState> getLatestCode() const override;

   private:
    common::Buffer code_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_CONST_WASM_PROVIDER
