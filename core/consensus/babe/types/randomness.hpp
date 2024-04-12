/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/sr25519_types.hpp"

namespace kagome::consensus::babe {

  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

}  // namespace kagome::consensus::babe
