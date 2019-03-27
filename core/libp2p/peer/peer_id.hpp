/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_HPP
#define KAGOME_PEER_ID_HPP

#include <memory>
#include <string>

#include "common/buffer.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::peer {
  /**
   * Peer of the network
   */
  class PeerId {
    /// so that private methods could be called from the manager
    friend class PeerIdManager;

   public:
    PeerId() = delete;

    /**
     * Get bytes representation of the Peer's id
     * @return bytes with the id
     */
    const kagome::common::Buffer &toBytes() const;

    /**
     * Get public key of the Peer
     * @return pointer to key; can be NULL
     */
    std::shared_ptr<crypto::PublicKey> publicKey() const;

    /**
     * Get private key of the Peer
     * @return pointer to key; can be NULL
     */
    std::shared_ptr<crypto::PrivateKey> privateKey() const;

    bool operator==(const PeerId &other) const;

   private:
    explicit PeerId(kagome::common::Buffer id);
    PeerId(kagome::common::Buffer id,
           std::shared_ptr<crypto::PublicKey> public_key,
           std::shared_ptr<crypto::PrivateKey> private_key);

    /**
     * Set a public key without performing any checks
     * @param public_key to be set
     */
    void unsafeSetPublicKey(std::shared_ptr<crypto::PublicKey> public_key);

    /**
     * Set a private key without performing any checks
     * @param private_key to be set
     */
    void unsafeSetPrivateKey(std::shared_ptr<crypto::PrivateKey> private_key);

    kagome::common::Buffer id_;
    std::shared_ptr<crypto::PublicKey> public_key_;
    std::shared_ptr<crypto::PrivateKey> private_key_;
  };
}  // namespace libp2p::peer

#endif  // KAGOME_PEER_HPP
