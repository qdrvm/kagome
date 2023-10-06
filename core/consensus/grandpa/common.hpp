/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>

#include "clock/clock.hpp"
#include "common/wrapper.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Id = crypto::Ed25519PublicKey;
  using Signature = crypto::Ed25519Signature;

  using BlockNumber = primitives::BlockNumber;
  using BlockHash = primitives::BlockHash;
  using BlockInfo = primitives::BlockInfo;

  using RoundNumber = uint64_t;
  using VoterSetId = uint64_t;

  using Clock = clock::SteadyClock;
  using Duration = Clock::Duration;
  using TimePoint = Clock::TimePoint;

}  // namespace kagome::consensus::grandpa
