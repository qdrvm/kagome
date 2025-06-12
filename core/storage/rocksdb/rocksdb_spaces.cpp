/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_spaces.hpp"

#include <algorithm>
#include <array>

#include <rocksdb/db.h>
#include <boost/assert.hpp>

namespace kagome::storage {

  static constexpr std::array kNames{"lookup_key",
                                     "header",
                                     "block_body",
                                     "justification",
                                     "trie_node",
                                     "dispute_data",
                                     "beefy_justification",
                                     "avaliability_storage",
                                     "audi_peers",
                                     "grandpa_historical_votes"};
  static_assert(kNames.size() == Space::kTotal - 1);

  std::string spaceName(Space space) {
    static const std::vector<std::string> names = []() {
      std::vector<std::string> names;
      names.push_back(rocksdb::kDefaultColumnFamilyName);
      names.insert(names.end(), kNames.begin(), kNames.end());
      return names;
    }();
    BOOST_ASSERT(space < Space::kTotal);
    return names.at(space);
  }

  std::optional<Space> spaceFromString(std::string_view string) {
    std::optional<Space> space;
    const auto it = std::ranges::find(kNames, string);
    if (it != std::end(kNames)) {
      space.emplace(static_cast<Space>(std::distance(std::begin(kNames), it)));
    }
    return space;
  }

}  // namespace kagome::storage
