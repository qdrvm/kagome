/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block_header.hpp"

using namespace kagome::common;

namespace kagome::primitives {

  BlockHeader::BlockHeader(Buffer parent_hash, size_t number, Buffer state_root,
                           Buffer extrinsics_root, Buffer digest)
      : parent_hash_(std::move(parent_hash)),
        number_(number),
        stateRoot_(std::move(state_root)),
        extrinsics_root_(std::move(extrinsics_root)),
        digest_(std::move(digest)) {}

  const Buffer &BlockHeader::parentHash() const {
    return parent_hash_;
  }

  uint64_t BlockHeader::number() const {
    return number_;
  }

  const Buffer &BlockHeader::stateRoot() const {
    return stateRoot_;
  }

  const Buffer &BlockHeader::extrinsicsRoot() const {
    return extrinsics_root_;
  }
  const Buffer &BlockHeader::digest() const {
    return digest_;
  }

}  // namespace kagome::primitives
