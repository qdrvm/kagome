/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block_header.hpp"

using kagome::common::Hash256;

namespace kagome::primitives {

  BlockHeader::BlockHeader(Hash256 parent_hash, size_t number,
                           Hash256 state_root, Hash256 extrinsics_root,
                           Digest digest)
      : parent_hash_(std::move(parent_hash)),
        number_(number),
        state_root_(std::move(state_root)),
        extrinsics_root_(std::move(extrinsics_root)),
        digest_(std::move(digest)) {}

  const Hash256 &BlockHeader::parentHash() const {
    return parent_hash_;
  }

  BlockNumber BlockHeader::number() const {
    return number_;
  }

  const Hash256 &BlockHeader::stateRoot() const {
    return state_root_;
  }

  const Hash256 &BlockHeader::extrinsicsRoot() const {
    return extrinsics_root_;
  }

  const Digest &BlockHeader::digest() const {
    return digest_;
  }

}  // namespace kagome::primitives
