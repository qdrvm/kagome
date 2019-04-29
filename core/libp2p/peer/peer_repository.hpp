/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_REPOSITORY_HPP
#define KAGOME_PEER_REPOSITORY_HPP

#include <memory>

#include "libp2p/peer/address_repository.hpp"
#include "libp2p/peer/key_repository.hpp"
#include "libp2p/peer/protocol_repository.hpp"

namespace libp2p::peer {

  /**
   * @brief Repository which stores all known information about peers, including
   * this peer.
   */
  class PeerRepository {
   public:
    PeerRepository(std::shared_ptr<AddressRepository> addrRepo,
                   std::shared_ptr<KeyRepository> keyRepo,
                   std::shared_ptr<ProtocolRepository> protocolRepo);

    /**
     * @brief Getter for an address repository.
     * @return associated instance of an address repository.
     */
    AddressRepository &getAddressRepository();

    /**
     * @brief Getter for a key repository.
     * @return associated instance of a key repository
     */
    KeyRepository &getKeyRepository();

    /**
     * @brief Getter for a protocol repository.
     * @return associated instance of a protocol repository.
     */
    ProtocolRepository &getProtocolRepository();

    /**
     * @brief Returns set of peer ids known by this peer repository.
     * @return unordered set of peers
     */
    std::unordered_set<PeerId> getPeers() const;

   private:
    std::shared_ptr<AddressRepository> addr_;
    std::shared_ptr<KeyRepository> key_;
    std::shared_ptr<ProtocolRepository> proto_;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PEER_REPOSITORY_HPP
