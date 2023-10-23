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

  struct GrandpaContext final {
    // Payload
    std::optional<libp2p::peer::PeerId> peer_id{};
    std::optional<network::VoteMessage> vote{};
    std::optional<network::CatchUpResponse> catch_up_response{};
    std::optional<network::FullCommitMessage> commit{};
    std::set<primitives::BlockInfo, std::greater<primitives::BlockInfo>>
        missing_blocks{};
    size_t checked_signature_counter = 0;
    size_t invalid_signature_counter = 0;
    size_t unknown_voter_counter = 0;

    GrandpaContext() = default;
    GrandpaContext(const GrandpaContext &) = default;
    GrandpaContext(GrandpaContext &&) = default;

    GrandpaContext &operator=(const GrandpaContext &) = default;
    GrandpaContext &operator=(GrandpaContext &&) = default;
  };

}  // namespace kagome::consensus::grandpa
