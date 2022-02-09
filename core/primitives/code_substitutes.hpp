/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODE_SUBSTITUTES_HPP
#define KAGOME_CODE_SUBSTITUTES_HPP

#include <initializer_list>
#include <unordered_set>

#include "block_id.hpp"

namespace kagome::primitives {

  /// A set of valid code_substitute ids.
  /// To get a code substitute you should get BlockInfo for this BlockId and
  /// pass it to fetchCodeSubstituteByBlockInfo()
  struct CodeSubstituteBlockIds
      : public std::unordered_set<primitives::BlockId> {
    CodeSubstituteBlockIds() = default;
    CodeSubstituteBlockIds(const CodeSubstituteBlockIds &src) = default;
    CodeSubstituteBlockIds(CodeSubstituteBlockIds &&src) = default;
    explicit CodeSubstituteBlockIds(
        std::initializer_list<primitives::BlockId> init)
        : unordered_set<primitives::BlockId>(init) {}
    ~CodeSubstituteBlockIds() = default;

    CodeSubstituteBlockIds &operator=(const CodeSubstituteBlockIds &src) =
        default;
    CodeSubstituteBlockIds &operator=(CodeSubstituteBlockIds &&src) = default;

    bool contains(const primitives::BlockInfo &block_info) const {
      return count(block_info.number) != 0 || count(block_info.hash) != 0;
    };

    bool contains(const primitives::BlockId &block_id) const {
      return count(block_id) != 0;
    }
  };

}  // namespace kagome::primitives

#endif  // KAGOME_CODE_SUBSTITUTES_HPP
