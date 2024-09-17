/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::storage::trie {
  /**
   * @brief TrieDbError enum provides error codes for TrieDb methods
   */
  enum class TrieError : uint8_t {
    NO_VALUE = 1,                 // no stored value found by the given key
    VALUE_RETRIEVE_NOT_PROVIDED,  // attempt to retrieve a value by hash with no
                                  // corresponding callback provided
    BROKEN_VALUE,  // value is absent from the database, although a node
                   // pointing to this value exists
  };
}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, TrieError)
