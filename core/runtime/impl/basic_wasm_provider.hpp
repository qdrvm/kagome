/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
#define KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP

#include "runtime/wasm_provider.hpp"

namespace test {

  class BasicWasmProvider : public kagome::runtime::WasmProvider {
   public:
    explicit BasicWasmProvider(std::string_view path);

    ~BasicWasmProvider() override = default;

    const kagome::common::Buffer &getStateCode() override;

   private:
    void initialize(std::string_view path);

    kagome::common::Buffer buffer_;
  };

}  // namespace test

#endif  // KAGOME_TEST_TESTUTIL_RUNTIME_BASIC_WASM_PROVIDER_HPP
