/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PUBLIC_KEY_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PUBLIC_KEY_IMPL_HPP

#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {

  /**
   * @class PublicKeyImpl implements PublicKey interface
   */
  class PublicKeyImpl : public PublicKey {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PublicKeyImpl(common::KeyType key_type, Buffer &&bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PublicKeyImpl(common::KeyType key_type, Buffer bytes);

    /**
     * @brief destructor
     */
    ~PublicKeyImpl() override = default;

    /**
     * @brief key type getter
     * @return key type
     */
    common::KeyType getType() const override;

    /**
     * @brief bytes getter
     * @return key content
     */
    const Buffer &getBytes() const override;

    bool operator==(const Key &other) const override;

   private:
    common::KeyType key_type_;  ///< key type
    Buffer bytes_;              ///< key content
  };

}  // namespace libp2p::crypto

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PUBLIC_KEY_IMPL_HPP
