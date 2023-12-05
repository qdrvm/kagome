/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <libp2p/peer/peer_info.hpp>
#include <vector>

#include "network/types/roles.hpp"
#include "primitives/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {

  using kagome::primitives::BlockHash;
  using kagome::primitives::BlockInfo;

  /**
   * Handshake sent when we open a block announces substream.
   * Is the structure to send to a new connected peer. It contains common
   * information about current peer and used by the remote peer to detect the
   * posibility of the correct communication with it.
   */
  struct BlockAnnounceHandshake {
    SCALE_TIE_ONLY(roles, best_block.number, best_block.hash, genesis_hash);

    Roles roles;  //!< Supported roles.

    primitives::BlockInfo best_block;  //!< Best block.

    BlockHash genesis_hash;  //!< Genesis block hash.
  };

}  // namespace kagome::network
