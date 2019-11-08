/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_ERROR_HPP
#define KAGOME_TRIE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::storage::trie {
  /**
   * @brief TrieDbError enum provides error codes for TrieDb methods
   */
  enum class TrieError {
    SUCCESS = 0,   // 0 should not represent an error
    NO_VALUE = 1,  // 1 not value in db
  };
}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TrieError)

#endif  // KAGOME_TRIE_ERROR_HPP
