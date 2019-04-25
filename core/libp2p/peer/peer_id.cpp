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
  }

  return "unknown error";
}

namespace libp2p::peer {
  PeerId::PeerId(multi::Multihash peer) : id_{std::move(peer)} {}

  template PeerId::FactoryResult PeerId::createPeerId<>(multi::Multihash &);
  template PeerId::FactoryResult PeerId::createPeerId<>(multi::Multihash &&);

  template <typename IdHash>
  PeerId::FactoryResult PeerId::createPeerId(IdHash &&peer_id) {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::kIdIsNotSha256Hash;
    }
    return PeerId{std::forward<IdHash>(peer_id)};
  }

  const multi::Multihash &PeerId::getPeerId() const {
    return id_;
  }
}  // namespace libp2p::peer
