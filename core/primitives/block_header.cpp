/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block_header.hpp"

namespace kagome::primitives {

  BlockHeader::BlockHeader(Buffer parentHash, Buffer stateRoot,
                           Buffer extrinsicsRoot, std::vector<Buffer> digest)
      : parentHash_(std::move(parentHash)),
        stateRoot_(std::move(stateRoot)),
        extrinsicsRoot_(std::move(extrinsicsRoot)),
        digest_(std::move(digest)) {}

  const Buffer &BlockHeader::parentHash() const {
    return parentHash_;
  }

  const Buffer &BlockHeader::stateRoot() const {
    return stateRoot_;
  }

  const Buffer &BlockHeader::extrinsicsRoot() const {
    return extrinsicsRoot_;
  }
  const std::vector<Buffer> &BlockHeader::digest() const {
    return digest_;
  }

}  // namespace kagome::primitives
