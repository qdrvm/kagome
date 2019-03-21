/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block.hpp"

namespace kagome::primitives {

  Block::Block(BlockHeader header, std::vector<Extrinsic> extrinsics)
      : header_(std::move(header)), extrinsics_(std::move(extrinsics)) {}

  const BlockHeader &Block::header() const {
    return header_;
  }

  const std::vector<Extrinsic> &Block::extrinsics() const {
    return extrinsics_;
  }

}  // namespace kagome::primitives
