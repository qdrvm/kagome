/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_HPP
#define KAGOME_KEY_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto {
  namespace common {
    enum class KeyType : uint32_t;
  }
  /**
   * Interface for public/private key
   */
  class Key {
   public:
    virtual ~Key() = default;
    /**
     * Get type of this key
     * @return the type
     */
    virtual common::KeyType getType() const = 0;

    /**
     * Get a byte representation of the key
     * @return the bytes
     */
    virtual const kagome::common::Buffer &getBytes() const = 0;

    virtual bool operator==(const Key &other) const = 0;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_KEY_HPP
