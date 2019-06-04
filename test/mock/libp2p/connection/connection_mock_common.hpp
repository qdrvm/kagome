/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MOCK_COMMON_HPP
#define KAGOME_CONNECTION_MOCK_COMMON_HPP

#include <ostream>
#include <vector>

#include "common/hexutil.hpp"
#include "libp2p/crypto/key.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"

/**
 * A couple of stream operators for values of results, without which the code
 * isn't going to compile
 */

namespace std {

  inline std::ostream &operator<<(std::ostream &s,
                                  const std::vector<unsigned char> &v) {
    s << kagome::common::hex_upper(v) << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s,
                                  const libp2p::multi::Multiaddress &m) {
    s << m.getStringAddress() << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s,
                                  const libp2p::crypto::PublicKey &key) {
    s << kagome::common::hex_upper(key.data) << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s,
                                  const libp2p::peer::PeerId &p) {
    s << p.toBase58() << "\n";
    return s;
  }

}  // namespace std

#endif  // KAGOME_CONNECTION_MOCK_COMMON_HPP
