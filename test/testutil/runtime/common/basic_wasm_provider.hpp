/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
#define KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP

#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {

  class BasicWasmProvider final : public kagome::runtime::WasmProvider {
   public:
    explicit BasicWasmProvider(std::string_view path);

    ~BasicWasmProvider() override = default;

    const common::Buffer &getStateCodeAt(
        const primitives::BlockHash &at) const override;

   private:
    void initialize(std::string_view path);

    kagome::common::Buffer buffer_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
