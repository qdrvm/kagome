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

    /**
     * @brief creates instance of specified random provider
     * some of them can fallback to pseudorandom sequence
     * if platform doesn't provide suitable source of random numbers
     * @param option random provider type
     * @return random provider instance
     */
    outcome::result<RandomProviderPtr> makeRandomProvider(RandomProviderType option);

    /**
     * @brief creates instance of default random provider
     * which generates cryptographic-safe random numbers
     * @return random provider instance
     */
    outcome::result<RandomProviderPtr> makeDefaultRandomProvider();
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP
