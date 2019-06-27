/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_LITERALS_HPP_
#define KAGOME_TEST_TESTUTIL_LITERALS_HPP_

#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multihash.hpp"

/// creates a buffer filled with characters from the original string
/// mind that it does not perform unhexing, there is ""_unhex for it
inline kagome::common::Buffer operator"" _buf(const char *c, size_t s) {
  std::vector<uint8_t> chars(c, c + s);
  return kagome::common::Buffer(std::move(chars));
}

inline kagome::common::Hash256 operator"" _hash256(const char *c, size_t s) {
  std::array<uint8_t, 32> bytes;
  bytes.fill(0);
  std::copy_n(c, std::min(s, 32ul), bytes.rbegin());
  kagome::common::Hash256 hash {std::move(bytes)};
  return hash;
}

inline std::vector<uint8_t> operator"" _v(const char *c, size_t s) {
  std::vector<uint8_t> chars(c, c + s);
  return chars;
}

inline kagome::common::Buffer operator"" _hex2buf(const char *c, size_t s) {
  return kagome::common::Buffer::fromHex(std::string_view(c, s)).value();
}

inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
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

#endif  // KAGOME_TEST_TESTUTIL_LITERALS_HPP_
