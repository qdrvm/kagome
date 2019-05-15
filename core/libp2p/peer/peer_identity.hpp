/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_IDENTITY_HPP
#define KAGOME_PEER_IDENTITY_HPP

#include <memory>
#include <string>
#include <string_view>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_info.hpp"

namespace libp2p::peer {
  /**
   * Identity of the given peer; includes its ID and multiaddress; usually is
   * passed over the network
   */
  class PeerIdentity {
    using FactoryResult = outcome::result<PeerIdentity>;

   public:
    enum class FactoryError { ID_EXPECTED = 1, NO_ADDRESSES, SHA256_EXPECTED };

    /**
     * Create a PeerIdentity from the string of format
     * "<multiaddress>/id/<base58_encoded_peer_id>"
     * @param identity - stringified identity, for instance,
     * "/ip4/192.168.0.1/tcp/1234/id/<ID>"
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(std::string_view identity);

    /**
     * Create a PeerIdentity from the PeerInfo structure
     * @param peer_info, from which identity is to be created; MUST contain both
     * PeerId and at least one Multiaddress; if there are several addresses in
     * the structure, a random one is chosen
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(const PeerInfo &peer_info);

    /**
     * Create a PeerIdentity from PeerId and Multiaddress
     * @param peer_id of the identity to be created
     * @param address of the identity to be created
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(const PeerId &peer_id,
                                const multi::Multiaddress &address);

    /**
     * Get a string representation of this identity;
     * "<multiaddress>/id/<base64-peer-id>"
     * @return identity string
     */
    std::string getIdentity() const;

    /**
     * Get a PeerId in this identity
     * @return peer id
     */
    const PeerId &getId() const noexcept;

    /**
     * Get a Multiaddress in this identity
     * @return multiaddress
     */
    const multi::Multiaddress &getAddress() const noexcept;

   private:
    /**
     * Construct a PeerIdentity instance
     * @param id, with which an instance is to be created
     * @param address, with which an instance is to be created
     */
    PeerIdentity(PeerId id, multi::Multiaddress address);

    PeerId id_;
    multi::Multiaddress address_;
  };
}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerIdentity::FactoryError)

#endif  // KAGOME_PEER_IDENTITY_HPP
