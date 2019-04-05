/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PRIVATE_KEY_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PRIVATE_KEY_IMPL_HPP

#include "libp2p/crypto/private_key.hpp"

namespace libp2p::crypto {

  /**
   * @class PrivateKeyImpl implements PrivateKey interface
   */
  class PrivateKeyImpl : public PrivateKey {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PrivateKeyImpl(common::KeyType key_type, Buffer &&bytes);

    /**
     * @brief constructor
     * @param key_type key type
     * @param bytes key content
     */
    PrivateKeyImpl(common::KeyType key_type, Buffer bytes);

    /**
     * @brief destructor
     */
    ~PrivateKeyImpl() override = default;

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

    /**
     * @brief derives public key from private one
     * @return derived public key
     */
    std::shared_ptr<PublicKey> publicKey() const override;

   private:
    common::KeyType key_type_;  ///< key type
    Buffer bytes_;              ///< ket content
  };

}  // namespace libp2p::crypto

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_PRIVATE_KEY_IMPL_HPP
