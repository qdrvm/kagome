/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

namespace {
  const libp2p::multi::MultibaseCodecImpl kCodec{};
  const kagome::hash::HasherImpl kHasher{};
}  // namespace

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
  using multi::MultibaseCodec;
  using multi::Multihash;

  PeerId::PeerId(multi::Multihash hash) : hash_{std::move(hash)} {}

  PeerId::FactoryResult PeerId::fromPublicKey(const crypto::PublicKey &key) {
    auto hash = kHasher.sha2_256(key.data);
    OUTCOME_TRY(
        multihash,
        Multihash::create(multi::sha256, Buffer{hash.begin(), hash.end()}));

    return PeerId{std::move(multihash)};
  }

  PeerId::FactoryResult PeerId::fromBase58(std::string_view id) {
    OUTCOME_TRY(decoded_id, kCodec.decode(id));
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
    return kCodec.encode(hash_.toBuffer(), MultibaseCodec::Encoding::BASE58);
  }

  const multi::Multihash &PeerId::toHash() const {
    return hash_;
  }
}  // namespace libp2p::peer
