/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MOCK_COMMON_HPP
#define KAGOME_CONNECTION_MOCK_COMMON_HPP

#include <ostream>
#include <vector>

#include <boost/functional/hash.hpp>
#include "libp2p/crypto/key.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"

/**
 * A couple of stream operators for values of results, without which the code
 * isn't going to compile
 */
namespace libp2p::connection {
  inline std::ostream &operator<<(std::ostream &s,
                                  const std::vector<unsigned char> &v) {
    s << boost::hash_value(v) << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s,
                                  const multi::Multiaddress &m) {
    s << m.getStringAddress() << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s,
                                  const crypto::PublicKey &key) {
    s << std::string(key.data.begin(), key.data.end()) << "\n";
    return s;
  }

  inline std::ostream &operator<<(std::ostream &s, const peer::PeerId &p) {
    s << p.toBase58() << "\n";
    return s;
  }
}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_MOCK_COMMON_HPP
