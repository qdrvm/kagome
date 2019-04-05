/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIVATE_KEY_HPP
#define KAGOME_PRIVATE_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  class PublicKey;
  /**
   * Represents private key
   */
  class PrivateKey : public Key {
   public:
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PrivateKey(common::KeyType key_type, Buffer &&bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PrivateKey(common::KeyType key_type, Buffer bytes);

    /**
     * @brief destructor
     */
    ~PrivateKey() override = default;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_PRIVATE_KEY_HPP
