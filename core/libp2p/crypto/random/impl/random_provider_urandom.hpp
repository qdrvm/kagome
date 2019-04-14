/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_URANDOM_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_URANDOM_HPP

#include "libp2p/crypto/random/random_provider.hpp"

namespace libp2p::crypto::random {
class RandomProviderURandom: public RandomProvider {
  using Buffer = kagome::common::Buffer;
 public:
  outcome::result<Buffer> randomBytes(size_t number) const override;
};
}

#endif //KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_IMPL_RANDOM_PROVIDER_URANDOM_HPP
