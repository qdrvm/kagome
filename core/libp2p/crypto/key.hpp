/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_CRYPTO_KEY_HPP
#define KAGOME_LIBP2P_CRYPTO_KEY_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto {

  namespace common {
    enum class KeyType : uint32_t;
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
    Key(common::KeyType key_type, Buffer &&bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    Key(common::KeyType key_type, const Buffer &bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
//    Key(common::KeyType key_type, Buffer bytes);

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

    virtual bool operator==(const Key &other) const;

   private:
    common::KeyType key_type_;  ///< key type
    Buffer bytes_;              ///< key content
  };

}  // namespace libp2p::crypto

#endif  // KAGOME_LIBP2P_CRYPTO_KEY_HPP
