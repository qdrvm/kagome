/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/fake_wasm_provider.hpp"

namespace kagome::runtime {
  FakeWasmProvider::FakeWasmProvider() {
    state_code_ = {1, 2, 3};
  }

}  // namespace kagome::runtime
