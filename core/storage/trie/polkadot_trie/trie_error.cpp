/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, TrieError, e) {
  using kagome::storage::trie::TrieError;
  switch (e) {
    case TrieError::NO_VALUE:
      return "no stored value found by the given key";
  }
  return "unknown";
}
