/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP

#include "libp2p/crypto/random/random_provider.hpp"

enum class RandomProviderType {
  URANDOM_PROVIDER,  ///< uses /dev/urandom as random source,
                     ///< suitable for most linux systems

  STD_RANDOM_DEVICE,  ///< uses std::random_device based works on all systems,
                      ///< though cannot be considered cryptographic-safe,
                      ///< because can fallback to cryptographic-unsafe prnd
                      ///< generators when platform doesn't support for
                      ///< real random source

  BOOST_RANDOM_DEVICE,  ///< uses boost::random_device // may not compile under
                        ///< cryptographic-unsafe platforms

  BSD_ENTROPY,  ///< uses bsd entropy method // only 256 bytes at max at a time
  BSD_ARC4,   ///< uses bsd arc4 algorythm for generating pseudo-random sequence
  BCRYPTGEN,  ///< uses bcryptgen library // for windows only
  OPENSSL,    ///< uses OpenSSL rand functions // not thread-safe
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
    outcome::result<RandomProviderPtr> makeRandomProvider(
        RandomProviderType option);

    /**
     * @brief creates instance of default random provider
     * which generates cryptographic-safe random numbers
     * @return random provider instance
     */
    outcome::result<RandomProviderPtr> makeDefaultRandomProvider();
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_FABRIC_HPP
