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
    static const std::array<std::string, Space::kTotal> names = {
        rocksdb::kDefaultColumnFamilyName,
        //        "lookup_key",
        //        "header",
        //        "block_data",
        //        "trie_node",
    };
    assert(space < Space::kTotal);
    static_assert(Space::kTotal == names.size());
    return names.at(space);
  }

}  // namespace kagome::storage
