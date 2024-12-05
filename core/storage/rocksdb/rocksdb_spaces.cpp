/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_spaces.hpp"

#include <array>
#include <algorithm>

#include <rocksdb/db.h>
#include <boost/assert.hpp>
#include <string_view>

namespace kagome::storage {
  static constexpr std::array kSpaceNames{"lookup_key",
                                          "header",
                                          "block_body",
                                          "justification",
                                          "trie_node",
                                          "trie_value",
                                          "dispute_data",
                                          "beefy_justification",
                                          "avaliability_storage"};

  std::string spaceName(Space space) {
    static_assert(kSpaceNames.size() == Space::kTotal - 1);

    static const std::vector<std::string> names = []() {
      std::vector<std::string> names;
      names.push_back(rocksdb::kDefaultColumnFamilyName);
      names.insert(names.end(), kSpaceNames.begin(), kSpaceNames.end());
      return names;
    }();
    BOOST_ASSERT(space < Space::kTotal);
    return names.at(space);
  }

  std::optional<Space> spaceByName(std::string_view space) {
    if (space == rocksdb::kDefaultColumnFamilyName) {
      return Space::kDefault;
    }
    auto it = std::ranges::find(kSpaceNames, space);
    if (it == kSpaceNames.end()) {
      return std::nullopt;
    }
    auto idx = it - kSpaceNames.begin() + 1;
    BOOST_ASSERT(idx < Space::kTotal);
    return static_cast<Space>(idx);
  }

}  // namespace kagome::storage
