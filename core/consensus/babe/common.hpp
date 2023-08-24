/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus::babe {

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFThreshold;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = common::Blob<crypto::constants::sr25519::vrf::OUTPUT_SIZE>;

}  // namespace kagome::consensus::babe

#endif  // KAGOME_COMMON_HPP
