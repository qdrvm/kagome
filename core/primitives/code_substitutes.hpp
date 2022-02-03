/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODE_SUBSTITUTES_HPP
#define KAGOME_CODE_SUBSTITUTES_HPP

#include <unordered_set>

#include "block_id.hpp"

namespace kagome::primitives {

  /// A set of valid code_substitute ids.
  /// To get a code substitute you should get BlockInfo for this BlockId and
  /// pass it to fetchCodeSubstituteByBlockInfo()
  using CodeSubstituteBlockIds = std::unordered_set<primitives::BlockId>;

}  // namespace kagome::primitives

#endif  // KAGOME_CODE_SUBSTITUTES_HPP
