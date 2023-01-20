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
    static const std::vector<std::string> names = {
        rocksdb::kDefaultColumnFamilyName,
        "lookup_key",
        "header",
        "justification",
        "block_data",
        "trie_node",
    };
    assert(Space::kTotal == names.size());
    assert(space < Space::kTotal);
    return names.at(space);
  }

}  // namespace kagome::storage
