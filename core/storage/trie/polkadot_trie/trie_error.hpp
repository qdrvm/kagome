/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_TRIE_ERROR_HPP
#define KAGOME_STORAGE_TRIE_TRIE_ERROR_HPP

#include "outcome/outcome.hpp"

namespace kagome::storage::trie {
  /**
   * @brief TrieDbError enum provides error codes for TrieDb methods
   */
  enum class TrieError {
    NO_VALUE = 1,                 // no stored value found by the given key
    VALUE_RETRIEVE_NOT_PROVIDED,  // attempt to retrieve a value by hash with no
                                  // corresponding callback provided
  };
}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TrieError)

#endif  // KAGOME_STORAGE_TRIE_TRIE_ERROR_HPP
