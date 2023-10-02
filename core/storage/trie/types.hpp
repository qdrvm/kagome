/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_TYPES_HPP
#define KAGOME_STORAGE_TRIE_TYPES_HPP

#include "common/blob.hpp"

namespace kagome::storage::trie {

  using RootHash = common::Hash256;

  enum class StateVersion { V0, V1 };

  /// blake2b_256(0x00)
  constexpr RootHash kEmptyRootHash{{
      0x03, 0x17, 0x0a, 0x2e, 0x75, 0x97, 0xb7, 0xb7, 0xe3, 0xd8, 0x4c,
      0x05, 0x39, 0x1d, 0x13, 0x9a, 0x62, 0xb1, 0x57, 0xe7, 0x87, 0x86,
      0xd8, 0xc0, 0x82, 0xf2, 0x9d, 0xcf, 0x4c, 0x11, 0x13, 0x14,
  }};

  constexpr uint8_t kEscapeCompactHeader = 1;
}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_TYPES_HPP
