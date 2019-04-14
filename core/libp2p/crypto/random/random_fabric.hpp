/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP

#include "libp2p/crypto/random/random_provider.hpp"

enum class RandomProviderType {
  URANDOM_PROVIDER,         ///< uses /dev/urandom as random source
  STD_RANDOM_PROVIDER,      ///< uses std::random_device
  BOOST_RANDOM_PROVIDER,    ///< uses boost::random_device
  BSD_RANDOM_PROVIDER,      ///< uses bsd methods
  BCRYPT_RANDOM_PROVIDER,   ///< uses bcryptgen library
  OPENSSL_RANDOM_PROVIDER,  ///< uses OpenSSL rand functions
};

namespace libp2p::crypto::random {
  class RandomFabric {
   public:
    using RandomProviderPtr = std::shared_ptr<RandomProvider>;
    outcome::result<RandomProviderPtr> makeRandomProvider(RandomProviderType option);
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP
