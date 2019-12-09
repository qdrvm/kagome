/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_FAKEWASMPROVIDER_HPP
#define KAGOME_CORE_RUNTIME_IMPL_FAKEWASMPROVIDER_HPP

#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {
  class FakeWasmProvider : public WasmProvider {
   public:
    FakeWasmProvider();

    ~FakeWasmProvider() override = default;

    const common::Buffer &getStateCode() override {
      return state_code_;
    }

   private:
    common::Buffer state_code_;  ///< state code
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_FAKEWASMPROVIDER_HPP
