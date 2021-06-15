/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/constant_code_provider.hpp"

namespace kagome::runtime {

  ConstantCodeProvider::ConstantCodeProvider(common::Buffer code)
      : code_{std::move(code)} {}

  outcome::result<gsl::span<const uint8_t>>
  ConstantCodeProvider::getCodeAt(const storage::trie::RootHash &at) const {
    return code_;
  }

}  // namespace kagome::runtime
