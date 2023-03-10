/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_spaces.hpp"

#include <array>
#include <cassert>

#include <rocksdb/db.h>

namespace kagome::storage {

  std::string spaceName(Space space) {
    static constexpr std::array kNames{
        "lookup_key",
        "header",
        "justification",
        "block_data",
        "trie_node",
        "trie_pruner",
    };
    static_assert(kNames.size() == Space::kTotal - 1);

    static const std::vector<std::string> names = []() {
      std::vector<std::string> names;
      names.push_back(rocksdb::kDefaultColumnFamilyName);
      names.insert(names.end(), kNames.begin(), kNames.end());
      return names;
    }();
    assert(space < Space::kTotal);
    return names.at(space);
  }

}  // namespace kagome::storage
