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
    Keys getEdKeys(KeyTypeId key_type) override;

    Keys getSrKeys(KeyTypeId key_type) override;

    void addEdKeyPair(KeyTypeId key_type, KeyPair key_pair) override;

    void addSrKeyPair(KeyTypeId key_type, KeyPair key_pair) override;

   private:
    std::map<KeyTypeId, KeyPair> ed_keys_;
    std::map<KeyTypeId, KeyPair> sr_keys_;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_IMPL_HPP
