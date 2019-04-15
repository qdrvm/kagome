/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/random_fabric.hpp"

#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/random/impl/random_provider_std.cpp"
#include "libp2p/crypto/random/impl/random_provider_boost.cpp"
#include "libp2p/crypto/random/impl/random_provider_urandom.hpp"


namespace libp2p::crypto::random {

  outcome::result<RandomFabric::RandomProviderPtr>
  RandomFabric::makeRandomProvider(RandomProviderType option) {
    switch (option) {
      case RandomProviderType::URANDOM_PROVIDER:
        return std::make_shared<RandomProviderURandom>();
      case RandomProviderType::STD_RANDOM_DEVICE:
        return std::make_shared<RandomProviderStd>();
      case RandomProviderType::BOOST_RANDOM_DEVICE:
        return std::make_shared<RandomProviderBoost>();
      case RandomProviderType::BSD_ENTROPY:
        return std::make_shared<RandomProviderBsdEntropy>();
      case RandomProviderType::BSD_ARC4:
        return std::make_shared<RandomProviderBsdArc4>();
      case RandomProviderType::OPENSSL:
        return std::make_shared<RandomProviderOpenssl>();
      default:
        break;
    }
    return RandomProviderError::INVALID_PROVIDER_TYPE;
  }

  outcome::result<RandomFabric::RandomProviderPtr>
  RandomFabric::makeDefaultRandomProvider() {
    std::terminate();
  }
}  // namespace libp2p::crypto::random
