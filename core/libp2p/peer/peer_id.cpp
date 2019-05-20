/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

#include "crypto/sha/sha256.hpp"
#include "libp2p/multi/multibase_codec/codecs/base58.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerId::FactoryError, e) {
  using E = libp2p::peer::PeerId::FactoryError;
  switch (e) {
    case E::SHA256_EXPECTED:
      return "expected a sha-256 multihash";
  }
  return "unknown error";
}

namespace libp2p::peer {
  using kagome::common::Buffer;
  using multi::Multihash;
  using multi::detail::decodeBase58;
  using multi::detail::encodeBase58;

  PeerId::PeerId(multi::Multihash hash) : hash_{std::move(hash)} {}

  PeerId::FactoryResult PeerId::fromPublicKey(const crypto::PublicKey &key) {
    auto hash = kagome::crypto::sha256(key.data.toVector());
    OUTCOME_TRY(
        multihash,
        Multihash::create(multi::sha256, Buffer{hash.begin(), hash.end()}));

    return PeerId{std::move(multihash)};
  }

  PeerId::FactoryResult PeerId::fromBase58(std::string_view id) {
    OUTCOME_TRY(decoded_id, decodeBase58(id));
    OUTCOME_TRY(hash, Multihash::createFromBuffer(decoded_id.toVector()));

    if (hash.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{std::move(hash)};
  }

  PeerId::FactoryResult PeerId::fromHash(const Multihash &hash) {
    if (hash.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    return PeerId{hash};
  }

  bool PeerId::operator==(const PeerId &other) const {
    return this->hash_ == other.hash_;
  }

  std::string PeerId::toBase58() const {
    return encodeBase58(hash_.toBuffer());
  }

  const multi::Multihash &PeerId::toMultihash() const {
    return hash_;
  }
}  // namespace libp2p::peer

size_t std::hash<libp2p::peer::PeerId>::operator()(
    const libp2p::peer::PeerId &peer_id) const {
  return std::hash<libp2p::multi::Multihash>()(peer_id.toMultihash());
}
