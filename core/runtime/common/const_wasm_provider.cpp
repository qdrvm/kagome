/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/const_wasm_provider.hpp"

namespace kagome::runtime {

  ConstWasmProvider::ConstWasmProvider(common::Buffer code)
      : code_{std::move(code)} {}

  const common::Buffer &ConstWasmProvider::getStateCodeAt(
      const primitives::BlockHash &at) const {
    return code_;
  }

}  // namespace kagome::runtime
