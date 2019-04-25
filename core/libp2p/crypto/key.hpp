/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_CRYPTO_KEY_HPP
#define KAGOME_LIBP2P_CRYPTO_KEY_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto {

  namespace common {
    enum class KeyType;
  }
  /**
   * Interface for public/private key
   */
  class Key {
   protected:
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    Key(common::KeyType key_type, Buffer bytes);

    virtual ~Key() = default;

    /**
     * Get type of this key
     * @return the type
     */
    virtual common::KeyType getType() const;

    /**
     * Get a byte representation of the key
     * @return the bytes
     */
    virtual const kagome::common::Buffer &getBytes() const;

   private:
    common::KeyType key_type_;  ///< key type
    Buffer bytes_;              ///< key content
  };

  /**
   * @brief compares keys
   * @param lhs first key
   * @param rhs second key
   * @return true if keys are equal false otherwise
   */
  bool operator==(const Key &lhs, const Key &rhs);

}  // namespace libp2p::crypto

#endif  // KAGOME_LIBP2P_CRYPTO_KEY_HPP
