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

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_TYPES_HPP
