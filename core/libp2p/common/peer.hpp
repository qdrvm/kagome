/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_HPP
#define KAGOME_PEER_HPP

#include <memory>
#include <optional>
#include <string_view>

#include "common/buffer.hpp"
#include "common/result.hpp"
#include "libp2p/common/peer_info.hpp"
#include "libp2p/security/key.hpp"

namespace libp2p::common {
  /**
   * Peer of the network
   */
  class Peer {
   private:
    using FactoryResult = kagome::expected::Result<Peer, std::string>;

   public:
    /**
     * Create a Peer instance
     * @param id of that peer - normally (but not necessary) a multihash of its
     * public key as a byte buffer
     */
    explicit Peer(const kagome::common::Buffer &id);
    explicit Peer(kagome::common::Buffer &&id);

    /**
     * Create a Peer instance
     * @param id of that peer - normally (but not necessary) a multihash of its
     * public key as a byte buffer
     * @param public_key of that peer; MUST be derived from the private key
     * @param private_key of that peer
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createPeer(const kagome::common::Buffer &id,
                                    const security::Key &public_key,
                                    const security::Key &private_key);

    /**
     * Create a peer instance from a public key
     * @param public_key to be in that instance; is used to derive peer id
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createPeer(const security::Key &public_key);

    /**
     * Create a peer instance from a private key
     * @param private_key to be in that instance, is used to derive public key
     * and id
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createPeer(const security::Key &private_key);

    /**
     * Create a peer from a base-encoded string with its id
     * @param id - base-encoded string
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createFromEncodedString(std::string_view id);

    /**
     * Create a peer from the public key
     * @param public_key - protobuf bytes of that peer's public key
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createFromPublicKey(
        const kagome::common::Buffer &public_key);

    /**
     * Create a peer from the private key
     * @param private_key - protobuf bytes of that peer's private key
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createFromPrivateKey(
        const kagome::common::Buffer &private_key);

    /**
     * Create a peer from the public key
     * @param public_key - base-encoded protobuf bytes of that peer's public key
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createFromPublicKey(std::string_view public_key);

    /**
     * Create a peer from the private key
     * @param private_key - base-encoded protobuf bytes of that peer's private
     * key
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createFromPrivateKey(std::string_view private_key);

    /**
     * Get hex representation of the Peer's id
     * @return hex string with the id
     */
    std::string_view toHex() const;

    /**
     * Get bytes representation of the Peer's id
     * @return bytes with the id
     */
    const kagome::common::Buffer &toBytes() const;

    /**
     * Get base58 representation of the Peer's id
     * @return base58-encoded string with the id
     */
    std::string_view toBase58() const;

    /**
     * Get public key of the Peer
     * @return optional with the key
     */
    std::optional<security::Key> publicKey() const;

    /**
     * Set public key of the Peer
     * @param public_key to be set
     * @return true, if it was set, false otherwise
     * @note if private key is set, this public key must be derived from it
     */
    bool setPublicKey(const security::Key &public_key);

    /**
     * Get private key of the Peer
     * @return optional with the key
     */
    std::optional<security::Key> privateKey() const;

    /**
     * Set private key of the Peer
     * @param private_key to be set
     * @return true, if it was set, false otherwise
     * @note if public key is set, this private key must derive the public key
     */
    bool setPrivateKey(const security::Key &private_key);

    /**
     * Get a Protobuf representation of the public key
     * @return optional with bytes of the key
     */
    std::optional<kagome::common::Buffer> marshalPublicKey() const;

    /**
     * Get a Protobuf representation of the private key
     * @return optional with bytes of the key
     */
    std::optional<kagome::common::Buffer> marshalPrivateKey() const;

    /**
     * Get this Peer's PeerInfo
     * @return peer info
     */
    const PeerInfo &peerInfo() const;

    /**
     * Get a string representation of that Peer
     * @return stringified Peer
     */
    std::string_view toString() const;

    bool operator==(const Peer &other) const;

   private:
    /// private, because we cannot allow keys not to be paired, so that private
    /// key does not derive public key
    Peer(const kagome::common::Buffer &id, const security::Key &public_key,
         const security::Key &private_key);

    kagome::common::Buffer id_;
    std::optional<security::Key> public_key_;
    std::optional<security::Key> private_key_;
    PeerInfo peer_info_;
  };
}  // namespace libp2p::common

#endif  // KAGOME_PEER_HPP
