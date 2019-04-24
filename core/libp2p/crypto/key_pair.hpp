/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_PAIR_HPP
#define KAGOME_KEY_PAIR_HPP

#include <boost/container_hash/hash.hpp>
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {

  struct KeyPair {
    PublicKey publicKey;
    PrivateKey privateKey;
  };

  inline bool operator==(const KeyPair &a, const KeyPair &b) {
    return a.publicKey == b.publicKey && a.privateKey == b.privateKey;
  }

}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::KeyPair> {
    size_t operator()(const libp2p::crypto::KeyPair &x) const {
      using libp2p::crypto::PrivateKey;
      using libp2p::crypto::PublicKey;
      size_t seed = 0;
      boost::hash_combine(seed, std::hash<PublicKey>()(x.publicKey));
      boost::hash_combine(seed, std::hash<PrivateKey>()(x.privateKey));
      return seed;
    }
  };
}  // namespace std

#endif  // KAGOME_KEY_PAIR_HPP
