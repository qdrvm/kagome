/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_IDENTITY_HPP
#define KAGOME_PEER_IDENTITY_HPP

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
    friend class PeerIdentityFactoryImpl;  // for private ctor

   public:
    /**
     * Get a string representation of this identity
     * @return identity string
     */
    std::string_view getIdentity() const noexcept;

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
     * @param identity - string, with which an instance is to be created
     * @param id, with which an instance is to be created
     * @param address, with which an instance is to be created
     */
    PeerIdentity(std::string identity, PeerId id, multi::Multiaddress address);

    std::string identity_;
    PeerId id_;
    multi::Multiaddress address_;
  };
}  // namespace libp2p::peer

#endif  // KAGOME_PEER_IDENTITY_HPP
