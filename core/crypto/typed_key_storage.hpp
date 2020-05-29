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
     * @brief returns all keys of provided type
     * @param key_type key type identifier
     */
    virtual Keys getKeys(KeyTypeId key_type) = 0;

    /**
     * @brief adds key pair to storage
     * @param key_pair key pair
     */
    virtual void addKeyPair(KeyPair key_pair) = 0;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_HPP
