/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/multi/multihash.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "common/blob.hpp"

using namespace kagome::common::literals;

inline kagome::common::Hash256 operator"" _hash256(const char *c, size_t s) {
  kagome::common::Hash256 hash{};
  std::copy_n(c, std::min(s, 32ul), hash.rbegin());
  return hash;
}

inline libp2p::peer::PeerId operator""_peerid(const char *c, size_t s) {
  //  libp2p::crypto::PublicKey p;
  auto data = std::vector<uint8_t>(c, c + s);
  libp2p::crypto::ProtobufKey pb_key(std::move(data));

  using libp2p::peer::PeerId;

  return PeerId::fromPublicKey(pb_key).value();
}
