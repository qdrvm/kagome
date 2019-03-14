/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_HPP
#define KAGOME_PEER_HPP

#include <string_view>
#include <memory>

#include <boost/optional/optional.hpp>
#include "common/buffer.hpp"
#include "common/result.hpp"
#include "libp2p/common/peer_info.hpp"
#include "libp2p/security/key.hpp"
#include "libp2p/multi/multibase_codec.hpp"

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
     * @param id of that peer - multihash of its public key as a byte buffer
     */
    explicit Peer(const kagome::common::Buffer &id);

    /**
     * Create a Peer instance
     * @param id of that peer - multihash of its public key as a byte buffer
     * @param public_key of that peer
     * @param private_key of that peer
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createPeer(const kagome::common::Buffer &id,
                                    const security::Key &public_key,
                                    const security::Key &private_key);

    /**
     * Create a Peer instance
     * @param public_key - buffer, containing its public key
     * @return Peer, if creation is successful, error otherwise
     */
    static FactoryResult createPeer(const kagome::common::Buffer &public_key);

    //    /**
    //     * Create a Peer instance
    //     * @param private_key - buffer, containing its private key
    //     * @return Peer, if creation is successful, error otherwise
    //     */
    //    static kagome::expected::Result<Peer, std::string> createPeer(
    //        const kagome::common::Buffer &private_key);

    static FactoryResult createFromHexString(std::string_view id);

    static FactoryResult createFromBytes(const kagome::common::Buffer &id);

    static FactoryResult createFromB58String(std::string_view id);

    static FactoryResult createFromPublicKey(const security::Key &public_key);

    static FactoryResult createFromPrivateKey(const security::Key &private_key);

    std::string_view toHex() const;

    const kagome::common::Buffer &toBytes() const;

    std::string_view toBase58() const;

    boost::optional<security::Key> publicKey() const;

    boost::optional<security::Key> privateKey() const;

    const PeerInfo &peerInfo() const;

    std::string_view toString() const;

    bool operator==(const Peer &other) const;

   private:
    /// private, because we cannot allow keys not to be paired, so that private
    /// key does not derive public key
    Peer(const kagome::common::Buffer &id, const security::Key &public_key,
         const security::Key &private_key);

    kagome::common::Buffer id_;
    boost::optional<security::Key> public_key_;
    boost::optional<security::Key> private_key_;
    PeerInfo peer_info_;

    std::unique_ptr<multi::MultibaseCodec> base_codec_;
  };
}  // namespace libp2p::common

#endif  // KAGOME_PEER_HPP
