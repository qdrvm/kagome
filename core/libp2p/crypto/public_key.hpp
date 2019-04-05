/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PUBLIC_KEY_HPP
#define KAGOME_PUBLIC_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  /**
   * Represents public key
   */
  class PublicKey : public Key {
   public:
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PublicKey(common::KeyType key_type, Buffer &&bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PublicKey(common::KeyType key_type, Buffer bytes);

    /**
     * @brief destructor
     */
    ~PublicKey() override = default;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_PUBLIC_KEY_HPP
