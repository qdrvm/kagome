/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_STD_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_STD_HPP

#include "libp2p/crypto/random/random_provider.hpp"

namespace libp2p::crypto::random {
  /**
   * @class RandomProviderStd utilizes std::random_device
   * crossplatform random numbers generator
   * main disadvantage is that on systems which don't provide
   * cryptographic-secure random sources it can fallback to
   * unsecure pseudo-random generators, and there is no API
   * allowing detect such situation.
   * Use it only if you know what you do.
   */
  class RandomProviderStd : public RandomProvider {
    using Buffer = kagome::common::Buffer;

   public:
    outcome::result<Buffer> randomBytes(size_t number) const override;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_STD_HPP
