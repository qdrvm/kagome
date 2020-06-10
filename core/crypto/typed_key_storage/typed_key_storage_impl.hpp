/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPED_KEY_STORAGE_IMPL_HPP
#define KAGOME_TYPED_KEY_STORAGE_IMPL_HPP

#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "crypto/typed_key_storage.hpp"

namespace kagome::crypto::storage {
  class TypedKeyStorageImpl : public TypedKeyStorage {
   public:
    EDKeys getEdKeys(KeyTypeId key_type) override;

    SRKeys getSrKeys(KeyTypeId key_type) override;

    void addEdKeyPair(KeyTypeId key_type,
                      const ED25519Keypair &key_pair) override;

    void addSrKeyPair(KeyTypeId key_type,
                      const SR25519Keypair &key_pair) override;

    boost::optional<ED25519Keypair> findE25519Key(
        KeyTypeId key_type, const ED25519PublicKey &pk) override;

    boost::optional<SR25519Keypair> findSr25519Key(
        KeyTypeId key_type, const SR25519PublicKey &pk) override;

   private:
    std::map<KeyTypeId, std::map<ED25519PublicKey, ED25519PrivateKey>> ed_keys_;
    std::map<KeyTypeId, std::map<SR25519PublicKey, SR25519SecretKey>> sr_keys_;
  };
}  // namespace kagome::crypto::storage

#endif  // KAGOME_TYPED_KEY_STORAGE_IMPL_HPP
