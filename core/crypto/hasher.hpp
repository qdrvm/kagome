/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_HASHER_HASHER_HPP_
#define KAGOME_CORE_HASHER_HASHER_HPP_

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::hash {
  class Hasher {
    using Hash128 = kagome::common::Hash128;
    using Hash256 = kagome::common::Hash256;
    using Buffer = kagome::common::Buffer;

   public:
    virtual ~Hasher() = default;

    /**
     * @brief hashTwox128 makes hash of source buffer
     * @param buffer source buffer
     * @return 128-bit hash value
     */
    virtual Hash128 hashTwox_128(const Buffer &buffer) = 0;

    /**
     * @brief hashTwox256 makes hash of source buffer
     * @param buffer source buffer
     * @return 256-bit hash value
     */
    virtual Hash256 hashTwox_256(const Buffer &buffer) = 0;

    /**
     * @brief
     * @param buffer
     * @return
     */
    virtual Hash256 hashBlake2_256(const Buffer &buffer) = 0;

    /**
     * @brief
     * @param buffer
     * @return
     */
    virtual Hash256 hashSha2_256(const Buffer &buffer) = 0;
  };
}  // namespace kagome::hash

#endif  // KAGOME_CORE_HASHER_HASHER_HPP_
