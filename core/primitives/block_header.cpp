/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block_header.hpp"

namespace kagome::primitives {

  void calculateBlockHash(const BlockHeader &header,
                          const crypto::Hasher &hasher) {
    header.updateHash(hasher);
  }

}  // namespace kagome::primitives
