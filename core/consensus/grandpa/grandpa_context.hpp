/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/structs.hpp"
#include "network/types/grandpa_message.hpp"

#include <set>

#include <libp2p/peer/peer_id.hpp>
#include <set>

namespace kagome::consensus::grandpa {
  using MissingBlocks =
      std::set<primitives::BlockInfo, std::greater<primitives::BlockInfo>>;

  struct GrandpaContext final {
    MissingBlocks missing_blocks{};
    size_t checked_signature_counter = 0;
    size_t invalid_signature_counter = 0;
    size_t unknown_voter_counter = 0;
  };

}  // namespace kagome::consensus::grandpa
