/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODE_SUBSTITUTES_HPP
#define KAGOME_CODE_SUBSTITUTES_HPP

#include <unordered_set>

namespace kagome::primitives {

  /// A set of valid code_substitute hashes.
  /// You can pass them to fetchCodeSubstituteByHash() and get code_substitute.
  using CodeSubstituteHashes = std::unordered_set<primitives::BlockHash>;

}  // namespace kagome::primitives

#endif  // KAGOME_CODE_SUBSTITUTES_HPP
