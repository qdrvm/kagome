/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_IMPL_HPP
#define KAGOME_CRYPTO_STORE_IMPL_HPP

#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  // TODO (yuraz): PRE-446 ECDSA will be added later
  using KeyPair = boost::variant<ED25519Keypair, SR25519Keypair>;
  using PublicKey = boost::variant<ED25519PublicKey, SR25519PublicKey>;
  using Seed = common::Hash256;

  struct CryptoStoreKeypair {
    KeyPair key_pair;
    KeyTypeId key_type;
  };

  enum class CryptoStoreError {
    WRONG_KEYFILE_NAME = 1,
    UNSUPPORTED_KEY_TYPE,
    UNSUPPORTED_CRYPTO_TYPE,
    NOT_REGULAR_FILE,
    FAILED_OPEN_FILE,
    INVALID_FILE_FORMAT,
    INCONSISTENT_KEYFILE,
    KEYS_PATH_IS_NOT_DIRECTORY
  };

  class CryptoStoreImpl : public CryptoStore {
   public:
    ~CryptoStoreImpl() override = default;

    CryptoStoreImpl(std::filesystem::path store_path,
                    std::shared_ptr<ED25519Provider> ed25519_provider,
                    std::shared_ptr<SR25519Provider> sr25519_provider,
                    std::shared_ptr<Secp256k1Provider> secp256k1_provider,
                    std::shared_ptr<Bip39Provider> bip39_provider);

    outcome::result<void> initialize(
        std::filesystem::path keys_directory) override;

    outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, const ED25519Seed &seed) override;

    outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, const SR25519Seed &seed) override;

    outcome::result<ED25519Keypair> generateEd25519KeyPair(
        KeyTypeId key_type) override;

    outcome::result<SR25519Keypair> generateSr25519KeyPair(
        KeyTypeId key_type) override;

    boost::optional<ED25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const ED25519PublicKey &pk) const override;

    boost::optional<SR25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const SR25519PublicKey &pk) const override;

    ED25519Keys getEd25519PublicKeys(KeyTypeId key_type) const override;

    SR25519Keys getSr25519PublicKeys(KeyTypeId key_type) const override;

   private:
    outcome::result<CryptoStoreKeypair> loadKeypair(
        const std::filesystem::path &key_path);

    std::filesystem::path keys_directory_;
    std::shared_ptr<ED25519Provider> ed25519_provider_;
    std::shared_ptr<SR25519Provider> sr25519_provider_;
    std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<Bip39Provider> bip39_provider_;

    std::map<KeyTypeId, std::map<ED25519PublicKey, ED25519PrivateKey>> ed_keys_;
    std::map<KeyTypeId, std::map<SR25519PublicKey, SR25519SecretKey>> sr_keys_;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, CryptoStoreError);

#endif  // KAGOME_CRYPTO_STORE_IMPL_HPP
