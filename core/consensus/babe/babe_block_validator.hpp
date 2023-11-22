/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block.hpp"

namespace kagome::consensus::babe {

  class BabeBlockValidator {
   public:
    virtual ~BabeBlockValidator() = default;

    virtual outcome::result<void> validateHeader(
        const primitives::BlockHeader &header) const = 0;
  };

}  // namespace kagome::consensus::babe
