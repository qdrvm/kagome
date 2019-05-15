/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_IDENTITY_FACTORY_HPP
#define KAGOME_PEER_IDENTITY_FACTORY_HPP

#include <string_view>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/identity/peer_identity.hpp"
#include "libp2p/peer/peer_info.hpp"

namespace libp2p::peer {
  /**
   * Create PeerIdentity
   */
  class PeerIdentityFactory {
   protected:
    using FactoryResult = outcome::result<PeerIdentity>;

   public:
    /**
     * Create a PeerIdentity from the string of format
     * "<multiaddress>/id/<base58_encoded_peer_id>"
     * @param identity - stringified identity, for instance,
     * "/ip4/192.168.0.1/tcp/1234/id/<ID>"
     * @return created object in case of success, error otherwise
     */
    virtual FactoryResult create(std::string_view identity) const = 0;

    /**
     * Create a PeerIdentity from the PeerInfo structure
     * @param peer_info, from which identity is to be created; MUST contain both
     * PeerId and at least one Multiaddress; if there are several addresses in
     * the structure, a random one is chosen
     * @return created object in case of success, error otherwise
     */
    virtual FactoryResult create(const PeerInfo &peer_info) const = 0;

    /**
     * Create a PeerIdentity from PeerId and Multiaddress
     * @param peer_id of the identity to be created
     * @param address of the identity to be created
     * @return created object in case of success, error otherwise
     */
    virtual FactoryResult create(const PeerId &peer_id,
                                 const multi::Multiaddress &address) const = 0;

    virtual ~PeerIdentityFactory() = default;
  };
}  // namespace libp2p::peer

#endif  // KAGOME_PEER_IDENTITY_FACTORY_HPP
