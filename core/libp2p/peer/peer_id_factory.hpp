/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_FACTORY_HPP
#define KAGOME_PEER_ID_FACTORY_HPP

#include <memory>
#include <string_view>

#include "common/buffer.hpp"
#include "common/result.hpp"
#include "libp2p/crypto/crypto_provider.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"
#include "libp2p/multi/multibase_codec.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::peer {
  /**
   * Create objects of type PeerId
   */
  class PeerIdFactory {
   private:
    using FactoryResult = kagome::expected::Result<PeerId, std::string>;

   public:
    /**
     * Create a PeerId factory
     * @param multibase_codec to be in this instance
     * @param crypto_provider to be in this instance
     */
    PeerIdFactory(const multi::MultibaseCodec &multibase_codec,
                  const crypto::CryptoProvider &crypto_provider);

    /**
     * Create a Peer instance
     * @param id of that peer - normally (but not necessary) a multihash of its
     * public key as a byte buffer
     */
    PeerId createPeerId(const kagome::common::Buffer &id);

    /**
     * Create a Peer instance
     * @param id of that peer - normally (but not necessary) a multihash of its
     * public key as a byte buffer
     */
    PeerId createPeerId(kagome::common::Buffer &&id);

    /**
     * Create a Peer instance
     * @param id of that peer - normally (but not necessary) a multihash of its
     * public key as a byte buffer
     * @param public_key of that peer const MUST be derived from the private key
     * @param private_key of that peer
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createPeerId(
        const kagome::common::Buffer &id,
        std::shared_ptr<crypto::PublicKey> public_key,
        std::shared_ptr<crypto::PrivateKey> private_key) const;

    /**
     * Create a peer instance from a public key
     * @param public_key to be in that instance; used to derive peer id
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPublicKey(
        std::shared_ptr<crypto::PublicKey> public_key) const;

    /**
     * Create a peer instance from a private key
     * @param private_key to be in that instance; used to derive public key
     * and id
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPrivateKey(
        std::shared_ptr<crypto::PrivateKey> private_key) const;

    /**
     * Create a peer from the public key
     * @param public_key - protobuf bytes of that peer's public key
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPublicKey(
        const kagome::common::Buffer &public_key) const;

    /**
     * Create a peer from the private key
     * @param private_key - protobuf bytes of that peer's private key
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPrivateKey(
        const kagome::common::Buffer &private_key) const;

    /**
     * Create a peer from the public key
     * @param public_key - base-encoded protobuf bytes of that peer's public key
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPublicKey(std::string_view public_key) const;

    /**
     * Create a peer from the private key
     * @param private_key - base-encoded protobuf bytes of that peer's private
     * key
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromPrivateKey(std::string_view private_key) const;

    /**
     * Create a peer from a base-encoded string with its id
     * @param id - base-encoded string
     * @return Peer, if creation is successful, error otherwise
     */
    FactoryResult createFromEncodedString(std::string_view id) const;

   private:
    const multi::MultibaseCodec &multibase_codec_;
    const crypto::CryptoProvider &crypto_provider_;
  };
}  // namespace libp2p::peer

#endif  // KAGOME_PEER_ID_FACTORY_HPP
