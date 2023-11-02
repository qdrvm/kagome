/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch_types.hpp"

namespace kagome::consensus::sassafras {

  using Randomness =
      common::Blob<crypto::constants::bandersnatch::vrf::OUTPUT_SIZE>;

}  // namespace kagome::consensus::sassafras
