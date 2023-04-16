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
        "block_body",
        "justification",
        "trie_node",
        "block_data",
    };
    assert(names.size() == Space::kTotal);
    assert(space < Space::kTotal);
    return names.at(space);
  }

}  // namespace kagome::storage
