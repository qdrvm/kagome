/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_LITERALS_HPP_
#define KAGOME_TEST_TESTUTIL_LITERALS_HPP_

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/multi/multihash.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/hexutil.hpp"

using namespace kagome::common::literals;

inline kagome::common::Hash256 operator"" _hash256(const char *c, size_t s) {
  kagome::common::Hash256 hash{};
  std::copy_n(c, std::min(s, 32ul), hash.rbegin());
  return hash;
}

inline std::vector<uint8_t> operator"" _v(const char *c, size_t s) {
  std::vector<uint8_t> chars(c, c + s);
  return chars;
}

inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
  if (s > 2 and c[0] == '0' and c[1] == 'x') {
    return kagome::common::unhexWith0x(std::string_view(c, s)).value();
  }
  return kagome::common::unhex(std::string_view(c, s)).value();
}

inline libp2p::multi::Multiaddress operator""_multiaddr(const char *c,
                                                        size_t s) {
  return libp2p::multi::Multiaddress::create(std::string_view(c, s)).value();
}

/// creates a multihash instance from a hex string
inline libp2p::multi::Multihash operator""_multihash(const char *c, size_t s) {
  return libp2p::multi::Multihash::createFromHex(std::string_view(c, s))
      .value();
}

inline libp2p::peer::PeerId operator""_peerid(const char *c, size_t s) {
  //  libp2p::crypto::PublicKey p;
  auto data = std::vector<uint8_t>(c, c + s);
  libp2p::crypto::ProtobufKey pb_key(std::move(data));

  using libp2p::peer::PeerId;

  return PeerId::fromPublicKey(pb_key).value();
}

#endif  // KAGOME_TEST_TESTUTIL_LITERALS_HPP_
