/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_OPERATORS_HPP
#define KAGOME_STREAM_OPERATORS_HPP

#include <vector>

#include "libp2p/crypto/key.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace std {
  std::ostream &operator<<(std::ostream &s,
                           const std::vector<unsigned char> &v) {
    s << std::string(v.begin(), v.end()) << "\n";
    return s;
  }

  std::ostream &operator<<(std::ostream &s,
                           const libp2p::multi::Multiaddress &m) {
    s << m.getStringAddress() << "\n";
    return s;
  }

  std::ostream &operator<<(std::ostream &s,
                           const libp2p::crypto::PublicKey &key) {
    s << std::string(key.data.begin(), key.data.end()) << "\n";
    return s;
  }
}  // namespace std

#endif  // KAGOME_STREAM_OPERATORS_HPP
