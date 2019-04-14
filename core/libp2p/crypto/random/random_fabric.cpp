/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/random_fabric.hpp"

#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/random/impl/random_provider_std.cpp"
#include "libp2p/crypto/random/impl/random_provider_urandom.hpp"

namespace libp2p::crypto::random {

  outcome::result<RandomFabric::RandomProviderPtr>
  RandomFabric::makeRandomProvider(RandomProviderType option) {
    switch (option) {
      case RandomProviderType::URANDOM_PROVIDER:
        return std::make_shared<RandomProviderURandom>();
      case RandomProviderType::STD_RANDOM_PROVIDER:
        return std::make_shared<RandomProviderStd>();
      default:
        break;
    }
    return RandomProviderError::INVALID_PROVIDER_TYPE;
  }
}  // namespace libp2p::crypto::random
