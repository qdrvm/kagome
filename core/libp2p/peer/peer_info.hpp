/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_INFO_HPP
#define KAGOME_PEER_INFO_HPP

#include <set>
#include <unordered_set>
#include <vector>

#include <gsl/span>
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::peer {
  /**
   * Information about the peer in the network
   */
  class PeerInfo {
   public:
    PeerInfo() = delete;
    PeerInfo(const PeerInfo &peer_info);
    PeerInfo &operator=(const PeerInfo &peer_info);
    PeerInfo(PeerInfo &&peer_info) noexcept;
    PeerInfo &operator=(PeerInfo &&peer_info) noexcept;

    /**
     * Create a PeerInfo instance
     * @param peer_id to be in new instance
     */
    explicit PeerInfo(PeerId peer_id);

    /**
     * Get PeerId of this PeerInfo
     * @return reference to PeerId - multihash
     */
    const PeerId &peerId() const;

    /**
     * Get protocols, which are supported by this PeerInfo
     * @return protocols
     */
    const std::unordered_set<multi::Multiaddress::Protocol>
        &supportedProtocols() const;

    /**
     * Get multiaddresses, through which this Peer can communicate
     * @return multiaddresses
     */
    const std::set<multi::Multiaddress> &multiaddresses() const;

    /**
     * Add protocols, through which this peer can communicate
     * @param protocols to be added
     * @return reference to updated PeerInfo
     */
    PeerInfo &addProtocols(gsl::span<multi::Multiaddress::Protocol> protocols);

    /**
     * Remove protocol, which this peer does not support anymore
     * @param protocol to be removed
     * @return true, if protocol was removed, false, if it was not found
     */
    bool removeProtocol(multi::Multiaddress::Protocol protocol);

    /**
     * Add multiaddresses, through which this peer can communication
     * @param multiaddresses to be added
     * @return reference to updated PeerInfo
     * @note use this, if you want to copy the addresses
     */
    PeerInfo &addMultiaddresses(gsl::span<multi::Multiaddress> multiaddresses);

    /**
     * Add multiaddresses, through which this peer can communicate
     * @param multiaddresses to be added
     * @return reference to updated PeerInfo
     * @note use this, if you want to move the addresses
     */
    PeerInfo &addMultiaddresses(
        std::vector<multi::Multiaddress> &&multiaddresses);

    /**
     * Remove multiaddress, which is no longer used by the peer
     * @param multiaddress to be removed
     * @return true, if address was removed, false, if it was not found
     */
    bool removeMultiaddress(const multi::Multiaddress &multiaddress);

    /**
     * Add multiaddress, through which this peer can communicate; actually adds
     * the address, if it is tried to be added for the second time
     * @param multiaddress to be added
     * @return true, if address was added, false otherwise
     */
    bool addMultiaddressSafe(const multi::Multiaddress &multiaddress);

    /**
     * Remove the provided multiaddresses and add another ones
     * @param to_remove - addresses to be removed
     * @param to_insert - addresses to be inserted
     * @return number of addresses, which were removed
     */
    size_t replaceMultiaddresses(gsl::span<multi::Multiaddress> to_remove,
                                 gsl::span<multi::Multiaddress> to_insert);

   private:
    PeerId peer_id_;
    std::unordered_set<multi::Multiaddress::Protocol> protocols_;
    /// ordered, because we need fast searches and removes
    std::set<multi::Multiaddress> multiaddresses_;
    std::vector<multi::Multiaddress> observed_multiaddresses_;
  };
}  // namespace libp2p::peer

#endif  // KAGOME_PEER_INFO_HPP
