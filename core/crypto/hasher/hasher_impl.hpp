/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
#define KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_

#include "crypto/hasher.hpp"

namespace kagome::hash {

  class HasherImpl : public Hasher {
    using Hash128 = kagome::common::Hash128;
    using Hash256 = kagome::common::Hash256;
    using Buffer = kagome::common::Buffer;

   public:
    ~HasherImpl() override = default;

    /**
     * @brief hashTwox128 makes hash of source buffer
     * @param buffer source buffer
     * @return 128-bit hash value
     */
    Hash128 hashTwox_128(const Buffer &buffer) override;

    /**
     * @brief hashTwox256 makes hash of source buffer
     * @param buffer source buffer
     * @return 256-bit hash value
     */
    Hash256 hashTwox_256(const Buffer &buffer) override;

    /**
     * @brief
     * @param buffer
     * @return
     */
    Hash256 hashBlake2_256(const Buffer &buffer) override;

    /**
     * @brief
     * @param buffer
     * @return
     */
    Hash256 hashSha2_256(const Buffer &buffer) override;
  };

}  // namespace kagome::hash

#endif  // KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
