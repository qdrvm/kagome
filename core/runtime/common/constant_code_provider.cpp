/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/constant_code_provider.hpp"

namespace kagome::runtime {

  ConstantCodeProvider::ConstantCodeProvider(common::Buffer code)
      : code_{std::move(code)} {}

  outcome::result<RuntimeCodeProvider::CodeAndItsState>
  ConstantCodeProvider::getCodeAt(const primitives::BlockInfo &at) const {
    return CodeAndItsState{code_, {}};
  }

  outcome::result<RuntimeCodeProvider::CodeAndItsState>
  ConstantCodeProvider::getLatestCode() const {
    return CodeAndItsState{code_, {}};
  }

}  // namespace kagome::runtime
