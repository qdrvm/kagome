/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, TrieError, e) {
  using kagome::storage::trie::TrieError;
  switch (e) {
    case TrieError::NO_VALUE:
      return "no stored value found by the given key";
    case TrieError::VALUE_RETRIEVE_NOT_PROVIDED:
      return "attempt to retrieve a value by hash with no corresponding "
             "callback provided";
  }
  return "unknown";
}
