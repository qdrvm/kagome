
/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_map>

#include "parachain/types.hpp"
#include "network/types/collator_messages_vstaging.hpp"

namespace kagome::parachain::statement_distribution {

    struct PeerState {
        network::View view;
        protocol_version: ValidationVersion,
        implicit_view: HashSet<Hash>,
        discovery_ids: Option<HashSet<AuthorityDiscoveryId>>,
    };


}
