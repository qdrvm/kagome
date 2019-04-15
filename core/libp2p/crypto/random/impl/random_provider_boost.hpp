/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_BOOST_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_BOOST_HPP

#include "libp2p/crypto/random/random_provider.hpp"

namespace libp2p::crypto::random {
  /**
   * @class RandomProviderBoost utilizes boost::random_device
   * crossplatform random numbers generator
   * may not compile under platforms which don't support for
   * cryptographic-secure random sources
   */

  class RandomProviderBoost : public RandomProvider {
    using Buffer = kagome::common::Buffer;

   public:
    outcome::result<Buffer> randomBytes(size_t number) const override;
  };

}  // namespace libp2p::crypto::random
#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_BOOST_HPP
