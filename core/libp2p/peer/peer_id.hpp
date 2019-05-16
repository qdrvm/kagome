/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_HPP
#define KAGOME_PEER_ID_HPP

#include <outcome/outcome.hpp>
#include "libp2p/crypto/key.hpp"
#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {

  /**
   * Unique identifier of the peer - SHA256 Multihash, in most cases, of its
   * public key
   */
  class PeerId {
    using FactoryResult = outcome::result<PeerId>;

   public:
    enum class FactoryError { SHA256_EXPECTED = 1 };

    static FactoryResult fromPublicKey(const crypto::PublicKey &key);

    static FactoryResult fromBase58(std::string_view id);

    static FactoryResult fromHash(const multi::Multihash &hash);

    std::string toBase58() const;

    const multi::Multihash &toHash() const;

    bool operator==(const PeerId &other) const;

   private:
    explicit PeerId(multi::Multihash hash);

    multi::Multihash hash_;
  };

}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerId::FactoryError)

#endif  // KAGOME_PEER_ID_HPP
