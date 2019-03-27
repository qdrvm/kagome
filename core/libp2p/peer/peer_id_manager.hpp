/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_MANAGER_HPP
#define KAGOME_PEER_ID_MANAGER_HPP

#include <memory>
#include <optional>
#include <string_view>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/crypto_provider.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"
#include "libp2p/multi/multibase_codec.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::peer {
  /**
   * Create objects of type PeerId
   */
  class PeerIdManager {
   private:
    using FactoryResult = outcome::result<PeerId>;

   public:
    /**
     * Create a PeerId manager
     * @param multibase_codec to be in this instance
     * @param crypto_provider to be in this instance
     */
    PeerIdManager(const multi::MultibaseCodec &multibase_codec,
                  const crypto::CryptoProvider &crypto_provider);

    /**
     * Possible factory errors
     */
    enum class FactoryError {
      kIdNotSHA256Hash,
      kEmptyId,
      kPubkeyIsNotDerivedFromPrivate,
      kIdIsNotHashOfPubkey,
      kCannotCreateIdFromPubkey,
      kCannotUnmarshalPubkey,
      kCannotUnmarshalPrivkey,
      kCannotDecodePubkey,
      kCannotDecodePrivkey,
      kCannotDecodeId
    };

    /**
     * Create a Peer instance
     * @param id of that peer - SHA-256 multihash of base-64-encoded public key
     */
    FactoryResult createPeerId(const kagome::common::Buffer &id) const;

    /**
     * Create a Peer instance
     * @param id of that peer - SHA-256 multihash of base-64-encoded public key
     */
    FactoryResult createPeerId(kagome::common::Buffer &&id) const;

    /**
     * Create a Peer instance
     * @param id of that peer - SHA-256 multihash of base-64-encoded public key
     * @param public_key of that peer; MUST be derived from the private key
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

    /**
     * Get hex representation of the Peer's id
     * @param peer, which is to be encoded
     * @return hex string with the id
     */
    std::string toHex(const PeerId &peer) const;

    /**
     * Get base58 representation of the Peer's id
     * @param peer, which is to be encoded
     * @return base58-encoded string with the id
     */
    std::string toBase58(const PeerId &peer) const;

    /**
     * Set public key of the Peer
     * @param peer, to which the key is to be set
     * @param public_key to be set; must be compliant with the id
     * @return true, if it was set, false otherwise
     * @note if private key is set, this public key must be derived from it
     * @note SHA256(base64(bytes(pubkey))) must be equal to the ID
     */
    bool setPublicKey(PeerId &peer,
                      std::shared_ptr<crypto::PublicKey> public_key) const;

    /**
     * Set private key of the Peer
     * @param peer, to which the key is to be set
     * @param private_key to be set; derived public key must be compliant with
     * the id
     * @return true, if it was set, false otherwise
     * @note if public key is set, this private key must derive the public key
     * @note SHA256(base64(bytes(privkey->pubkey))) must be equal to the ID
     */
    bool setPrivateKey(PeerId &peer,
                       std::shared_ptr<crypto::PrivateKey> private_key) const;

    /**
     * Get a Protobuf representation of the Peer's public key
     * @param peer, from which the key should be taken
     * @return optional with bytes of the key
     */
    std::optional<kagome::common::Buffer> marshalPublicKey(
        const PeerId &peer) const;

    /**
     * Get a Protobuf representation of the Peer's private key
     * @param peer, from which the key should be taken
     * @return optional with bytes of the key
     */
    std::optional<kagome::common::Buffer> marshalPrivateKey(
        const PeerId &peer) const;

    /**
     * Get a string representation of that Peer
     * @return stringified Peer
     */
    std::string toString(const PeerId &peer) const;

   private:
    /**
     * Check, if ID of that peer is derived from the given public key
     * @param peer, whose id is to be checked
     * @param key to be checked
     * @return true, if SHA256(base64(bytes(pubkey))) == key, false otherwise
     */
    bool idDerivedFromPublicKey(const PeerId &peer,
                                const libp2p::crypto::PublicKey &key) const;

    /**
     * Convert public key to an ID by encoding to base64 and SHA-256 hashing the
     * result
     * @param key to be converted
     * @return resulting buffer, if key was successfully converted, none
     * otherwise
     * @note static class member to be used by the factory as well
     */
    std::optional<kagome::common::Buffer> idFromPublicKey(
        const libp2p::crypto::PublicKey &key) const;

    const multi::MultibaseCodec &multibase_codec_;
    const crypto::CryptoProvider &crypto_provider_;
  };
}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR_2(libp2p::peer, PeerIdManager::FactoryError)

#endif  // KAGOME_PEER_ID_MANAGER_HPP
