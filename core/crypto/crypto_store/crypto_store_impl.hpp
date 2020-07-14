/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_IMPL_HPP
#define KAGOME_CRYPTO_STORE_IMPL_HPP

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "common/logger.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

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
    WRONG_SEED_SIZE,
    KEY_NOT_FOUND,
    KEYS_PATH_IS_NOT_DIRECTORY,
    FAILED_CREATE_KEYS_DIRECTORY
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
     * @brief ensures that keys directory exists
     * @param keys_directory keys directory
     * @return success if exists or managed to create, failure otherwise
     */
    outcome::result<void> initialize(Path keys_directory);

    outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    ED25519Keypair generateEd25519Keypair(KeyTypeId key_type,
                                          const ED25519Seed &seed) override;

    SR25519Keypair generateSr25519Keypair(KeyTypeId key_type,
                                          const SR25519Seed &seed) override;

    outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type) override;

    outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type) override;

    outcome::result<ED25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const ED25519PublicKey &pk) const override;

    outcome::result<SR25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const SR25519PublicKey &pk) const override;

    ED25519Keys getEd25519PublicKeys(KeyTypeId key_type) const override;

    SR25519Keys getSr25519PublicKeys(KeyTypeId key_type) const override;

   private:
    outcome::result<std::pair<KeyTypeId, store::PublicKey>> parseKeyFileName(
        std::string_view file_name) const;

    Path composeKeyPath(KeyTypeId key_type,
                        const store::PublicKey &public_key) const;

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
    common::Logger logger_;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, CryptoStoreError);

#endif  // KAGOME_CRYPTO_STORE_IMPL_HPP
