/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_IMPL_HPP
#define KAGOME_CRYPTO_STORE_IMPL_HPP

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  // TODO (yuraz): PRE-446 ECDSA will be added later
  namespace store {
    using KeyPair = boost::variant<ED25519Keypair, SR25519Keypair>;
    using PublicKey = common::Blob<32>;
    using Seed = common::Hash256;

    enum class CryptoId { ED25519, SR25519, SECP256k1 };
  }  // namespace store

  enum class CryptoStoreError {
    WRONG_KEYFILE_NAME = 1,
    UNSUPPORTED_KEY_TYPE,
    UNSUPPORTED_CRYPTO_TYPE,
    NOT_REGULAR_FILE,
    FAILED_OPEN_FILE,
    FILE_DOESNT_EXIST,
    INVALID_FILE_FORMAT,
    INCONSISTENT_KEYFILE,
    FAILED_CREATE_DIRECTORY,
    KEYS_PATH_IS_NOT_DIRECTORY,
    WRONG_SEED_SIZE
  };

  class CryptoStoreImpl : public CryptoStore {
   public:
    // currently std::filesystem::path is missing required methods in macos SDK

    ~CryptoStoreImpl() override = default;

    CryptoStoreImpl(std::shared_ptr<ED25519Provider> ed25519_provider,
                    std::shared_ptr<SR25519Provider> sr25519_provider,
                    std::shared_ptr<Secp256k1Provider> secp256k1_provider,
                    std::shared_ptr<Bip39Provider> bip39_provider,
                    std::shared_ptr<CSPRNG> random_generator);

    /**
     * @brief sets keys directory up and checks that it is correct
     */
    outcome::result<void> initialize(Path keys_directory) override;

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

    outcome::result<ED25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const ED25519PublicKey &pk) const override;

    outcome::result<SR25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const SR25519PublicKey &pk) const override;

    outcome::result<ED25519Keys> getEd25519PublicKeys(
        KeyTypeId key_type) const override;

    outcome::result<SR25519Keys> getSr25519PublicKeys(
        KeyTypeId key_type) const override;

   private:
    Path composeKeyPath(KeyTypeId key_type,
                        const store::PublicKey &public_key) const;

    outcome::result<store::KeyPair> loadKeypair(store::CryptoId crypto_id,
                                                const Path &key_path);

    outcome::result<std::string> loadFile(const Path &file_name) const;

    outcome::result<void> storeKeyfile(KeyTypeId key_type,
                                       const store::PublicKey &public_key,
                                       const store::Seed &seed);

    Path keys_directory_;
    std::shared_ptr<ED25519Provider> ed25519_provider_;
    std::shared_ptr<SR25519Provider> sr25519_provider_;
    std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<Bip39Provider> bip39_provider_;
    std::shared_ptr<CSPRNG> random_generator_;

    std::map<KeyTypeId, std::map<ED25519PublicKey, ED25519PrivateKey>> ed_keys_;
    std::map<KeyTypeId, std::map<SR25519PublicKey, SR25519SecretKey>> sr_keys_;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, CryptoStoreError);

#endif  // KAGOME_CRYPTO_STORE_IMPL_HPP
