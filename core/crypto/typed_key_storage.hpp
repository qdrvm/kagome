/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPED_KEY_STORAGE_HPP
#define KAGOME_TYPED_KEY_STORAGE_HPP

#include "crypto/key_type.hpp"

#include <boost/optional.hpp>

namespace kagome::crypto::storage {

  class TypedKeyStorage {
   public:
    virtual ~TypedKeyStorage() = default;

    using EDKeys = std::vector<ED25519PublicKey>;
    using SRKeys = std::vector<SR25519PublicKey>;
    /**
     * @brief returns all ed25519 keys of provided type
     * @param key_type key type identifier
     */
    virtual EDKeys getEdKeys(KeyTypeId key_type) = 0;

    /**
     * @brief returns all sr25519 keys of provided type
     * @param key_type key type identifier
     */
    virtual SRKeys getSrKeys(KeyTypeId key_type) = 0;

    /**
     * @brief adds ed25519 key pair to storage
     * @param key_pair key pair
     */
    virtual void addEdKeyPair(KeyTypeId key_type,
                              const ED25519Keypair &key_pair) = 0;

    /**
     * @brief adds sr25519 key pair to storage
     * @param key_pair key pair
     */
    virtual void addSrKeyPair(KeyTypeId key_type,
                              const SR25519Keypair &key_pair) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual boost::optional<ED25519Keypair> findEdKey(
        KeyTypeId key_type, const ED25519PublicKey &pk) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual boost::optional<SR25519Keypair> findSrKey(
        KeyTypeId key_type, const SR25519PublicKey &pk) = 0;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_HPP
