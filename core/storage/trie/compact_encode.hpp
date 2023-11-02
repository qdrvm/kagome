/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/on_read.hpp"

namespace kagome::storage::trie {
  outcome::result<common::Buffer> compactEncode(const OnRead &db,
                                                const common::Hash256 &root);
}  // namespace kagome::storage::trie
