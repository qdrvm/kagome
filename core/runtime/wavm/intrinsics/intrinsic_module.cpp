/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/intrinsics/intrinsic_module.hpp"

namespace kagome::runtime::wavm {
  WAVM::IR::MemoryType IntrinsicModule::kIntrinsicMemoryType = {
      WAVM::IR::MemoryType(false, WAVM::IR::IndexType::i32, {21, UINT64_MAX})};
}