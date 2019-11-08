/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, TrieError, e) {
  using kagome::storage::trie::TrieError;
  switch (e) {
    case TrieError::SUCCESS:
      return "success";
    case TrieError::NO_VALUE:
      return "no value";
    default:
      return "unknown";
  }
}
