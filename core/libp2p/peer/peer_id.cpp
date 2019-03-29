/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerId::FactoryError, e) {
  using Error = libp2p::peer::PeerId::FactoryError;
  switch (e) {
    case Error::kIdIsNotSha256Hash:
      return "provided id is not a SHA-256 multihash";
    default:
      return "unknown error";
  }
}

namespace libp2p::peer {
  PeerId::PeerId(const PeerId &peer_id) = default;
  PeerId &PeerId::operator=(const PeerId &peer_id) = default;
  PeerId::PeerId(PeerId &&peer_id) noexcept = default;
  PeerId &PeerId::operator=(PeerId &&peer_id) noexcept = default;

  PeerId::PeerId(multi::Multihash peer) : id_{std::move(peer)} {}

  PeerId::FactoryResult PeerId::createPeerId(const multi::Multihash &peer_id) {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::kIdIsNotSha256Hash;
    }
    return PeerId{peer_id};
  }

  PeerId::FactoryResult PeerId::createPeerId(multi::Multihash &&peer_id) {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::kIdIsNotSha256Hash;
    }
    return PeerId{std::move(peer_id)};
  }

  const multi::Multihash &PeerId::peerId() const {
    return id_;
  }
}  // namespace libp2p::peer
