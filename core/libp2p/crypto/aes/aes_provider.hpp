/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto::aes {
  /**
   * @class AesCrypt provides methods for encryption and decryption by means of
   * AES128 and AES256
   */
  class AesProvider {
    using Buffer = kagome::common::Buffer;
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

   public:
    /**
     * @brief encrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    outcome::result<Buffer> encrypt_128_ctr(const Aes128Secret &secret,
                                            const Buffer &data) const;

    /**
     * @brief decrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    outcome::result<Buffer> decrypt_128_ctr(const Aes128Secret &secret,
                                            const Buffer &data) const;

    /**
     * @brief encrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    outcome::result<Buffer> encrypt_256_ctr(const Aes256Secret &secret,
                                            const Buffer &data) const;

    /**
     * @brief decrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    outcome::result<Buffer> decrypt_256_ctr(const Aes256Secret &secret,
                                            const Buffer &data) const;
  };
}  // namespace libp2p::crypto::aes

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
