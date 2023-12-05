/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "libp2p/crypto/random_generator.hpp"

namespace kagome::crypto {
  using RandomGenerator = libp2p::crypto::random::RandomGenerator;
  using CSPRNG = libp2p::crypto::random::CSPRNG;
}  // namespace kagome::crypto
