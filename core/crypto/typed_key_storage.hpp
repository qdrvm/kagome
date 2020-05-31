/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPED_KEY_STORAGE_HPP
#define KAGOME_TYPED_KEY_STORAGE_HPP

#include "crypto/key_type.hpp"

namespace kagome::crypto::storage {

  class TypedKeyStorage {
   public:
    virtual ~TypedKeyStorage() = default;

    using Keys = std::vector<KeyPair>;
    /**
     * @brief returns all ed25519 keys of provided type
     * @param key_type key type identifier
     */
    virtual Keys getEdKeys(KeyTypeId key_type) = 0;

    /**
     * @brief returns all sr25519 keys of provided type
     * @param key_type key type identifier
     */
    virtual Keys getSrKeys(KeyTypeId key_type) = 0;

    /**
     * @brief adds ed25519 key pair to storage
     * @param key_pair key pair
     */
    virtual void addEdKeyPair(KeyTypeId key_type, KeyPair key_pair) = 0;

    /**
     * @brief adds sr25519 key pair to storage
     * @param key_pair key pair
     */
    virtual void addSrKeyPair(KeyTypeId key_type, KeyPair key_pair) = 0;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_HPP
