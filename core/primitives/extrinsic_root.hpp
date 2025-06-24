/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/range/adaptor/transformed.hpp>
#include <scale/scale.hpp>

#include "crypto/blake2/blake2b.h"
#include "primitives/block.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

namespace kagome::primitives {
  inline auto extrinsicRoot(const BlockBody &body) {
    return calculateOrderedTrieHash(
               storage::trie::StateVersion::V0,
               body | boost::adaptors::transformed([](const Extrinsic &ext) {
                 return common::Buffer{scale::encode(ext).value()};
               }),
               crypto::blake2b)
        .value();
  }
  inline bool checkExtrinsicRoot(const BlockHeader &header,
                                 const BlockBody &body) {
    return extrinsicRoot(body) == header.extrinsics_root;
  }
}  // namespace kagome::primitives
