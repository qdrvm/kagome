/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_PROVIDER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_PROVIDER_HPP

#include <random>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::crypto::random {
  class RandomProvider {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief obtains from system and returns random bytes
     * @param number number of bytes
     * @return random bytes buffer of specified size
     */
    virtual outcome::result<Buffer> randomBytes(size_t number) const = 0;
  };
}  // namespace libp2p::crypto::random

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_RANDOM_RANDOM_PROVIDER_HPP
