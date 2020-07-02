/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "testutil/storage/base_fs_test.hpp"

using kagome::crypto::Bip39Provider;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStore;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;

CryptoStoreImpl::Path crypto_store_test_directory =
    boost::filesystem::temp_directory_path().append("crypto_store_test");

struct CryptoStoreTest : public test::BaseFS_Test {
  CryptoStoreTest() : BaseFS_Test(crypto_store_test_directory) {}

  void SetUp() override {
    auto ed25519_provider = std::make_shared<ED25519ProviderImpl>();
    auto csprng = std::make_shared<BoostRandomGenerator>();
    auto sr25519_provider = std::make_shared<SR25519ProviderImpl>(csprng);
    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();

    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider));
    crypto_store =
        std::make_shared<CryptoStoreImpl>(std::move(ed25519_provider),
                                          std::move(sr25519_provider),
                                          std::move(secp256k1_provider),
                                          std::move(bip39_provider),
                                          std::move(csprng));
  }

  std::shared_ptr<CryptoStore> crypto_store;
};
