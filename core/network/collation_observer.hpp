/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATION_OBSERVER_HPP
#define KAGOME_COLLATION_OBSERVER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "consensus/grandpa/common.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /**
   * Reacts to messages, related to collation protocol
   */
  struct CollationObserver {
    virtual ~CollationObserver() = default;

    /**
     * Triggered when a Peer makes advertisement
     * @param para_hash hash of the parachain block
     */
    virtual void onAdvertise(primitives::BlockHash para_hash) = 0;

    /**
     * Triggered when a Peer declares as a collator
     */
    virtual void onDeclare(
        consensus::grandpa::Id collator_pubkey,
        uint32_t para_id,
        consensus::grandpa::Signature collator_signature) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_COLLATION_OBSERVER_HPP
