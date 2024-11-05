/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_spaces.hpp"

#include <array>

#include <rocksdb/db.h>
#include <boost/assert.hpp>

namespace kagome::storage {

  std::string spaceName(Space space) {
    static constexpr std::array kNames{
        "lookup_key",
        "header",
        "block_body",
        "justification",
        "trie_node",
        "trie_value",
        "dispute_data",
        "beefy_justification",
    };
    static_assert(kNames.size() == Space::kTotal - 1);

    static const std::vector<std::string> names = []() {
      std::vector<std::string> names;
      names.push_back(rocksdb::kDefaultColumnFamilyName);
      names.insert(names.end(), kNames.begin(), kNames.end());
      return names;
    }();
    BOOST_ASSERT(space < Space::kTotal);
    return names.at(space);
  }

}  // namespace kagome::storage
