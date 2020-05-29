/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPED_KEY_STORAGE_IMPL_HPP
#define KAGOME_TYPED_KEY_STORAGE_IMPL_HPP

#include "crypto/typed_key_storage.hpp"

namespace kagome::crypto::storage {
  class TypedKeyStorageImpl : public TypedKeyStorage {
   public:
    Keys getKeys(KeyTypeId key_type) override;

    void addKeyPair(KeyPair key_pair) override;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_IMPL_HPP
