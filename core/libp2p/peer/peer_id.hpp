/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_HPP
#define KAGOME_PEER_ID_HPP

#include <outcome/outcome.hpp>
#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {
  /**
   * Stores ID of the Peer
   */
  class PeerId {
   private:
    using FactoryResult = outcome::result<PeerId>;

   public:
    PeerId() = delete;

    enum class FactoryError { kIdIsNotSha256Hash = 1 };
    /**
     * Create a PeerId instance
     * @tparam IdHash - Multihash type
     * @param peer_id - ID of the peer; must be a SHA-256 multihash
     * @return new instance of PeerId in case of success, error otherwise
     */
    template <typename IdHash>
    static FactoryResult createPeerId(IdHash &&peer_id);

    /**
     * Get PeerId of this instance
     * @return multihash peer id
     */
    const multi::Multihash &getPeerId() const;

   private:
    /**
     * Create from the peer id multihash
     * @param peer to be put in this instance
     */
    explicit PeerId(multi::Multihash peer);

    multi::Multihash id_;
  };
}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerId::FactoryError)

#endif  // KAGOME_PEER_ID_HPP
