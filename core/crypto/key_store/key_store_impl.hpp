/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include "common/blob.hpp"
#include "common/optref.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/key_store.hpp"
#include "crypto/key_store/crypto_suites.hpp"
#include "crypto/key_store/key_cache.hpp"
#include "crypto/key_store/key_file_storage.hpp"
#include "filesystem/common.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "utils/read_file.hpp"

namespace kagome::crypto {

  /// TODO(Harrm) Add policies to emit a warning when found a keypair
  /// with incompatible type and algorithm (e. g. ed25519 BABE keypair,
  /// whereas BABE has to be sr25519 only) or when trying to generate more
  /// keypair than there should be (e. g. more than one libp2p keypair is a
  /// suspicious behaviour)
  template <Suite T>
  class KeySuiteStoreImpl : public KeySuiteStore<T> {
   public:
    using Keypair = typename T::Keypair;
    using PrivateKey = typename T::PrivateKey;
    using PublicKey = typename T::PublicKey;
    using Seed = typename T::Seed;

    KeyStoreImpl(std::shared_ptr<T> suite,
                 std::shared_ptr<Bip39Provider> bip39_provider,
                 std::shared_ptr<CSPRNG> csprng,
                 std::shared_ptr<KeyFileStorage> key_fs)
        : file_storage_{std::move(key_fs)},
          suite_{std::move(suite)},
          bip39_provider_{std::move(bip39_provider)},
          csprng_{std::move(csprng)},
          logger_{log::createLogger("KeyStore", "key_store")} {
      BOOST_ASSERT(suite_ != nullptr);
      BOOST_ASSERT(bip39_provider_ != nullptr);
      BOOST_ASSERT(file_storage_ != nullptr);
      BOOST_ASSERT(csprng_ != nullptr);
    }

    outcome::result<Keypair> generateKeypair(
        KeyType key_type, std::string_view mnemonic_phrase) override {
      OUTCOME_TRY(bip, bip39_provider_->generateSeed(mnemonic_phrase));
      auto kp = suite_->generateKeypair(bip);
      keys_[key_type].insert(kp.public_key, std::move(kp.secret_key));
      return kp;
    }

    outcome::result<const Keypair &> generateKeypair(
        KeyType key_type, const Seed &seed) override {
      OUTCOME_TRY(kp, suite_->generateKeypair(seed, {}));
      keys_[key_type].insert(kp.public_key, std::move(kp.secret_key));
      return keys_[key_type][kp.public_key];
    }

    outcome::result<const Keypair &> generateKeypairOnDisk(
        KeyType key_type) override {
      SecureBuffer<> seed_buf(Seed::size());
      csprng_->fillRandomly(seed_buf);
      OUTCOME_TRY(seed, Seed::from(std::move(seed_buf)));
      OUTCOME_TRY(kp, suite_->generateKeypair(seed, {}));
      keys_[key_type].insert(kp.public_key, std::move(kp.secret_key));
      OUTCOME_TRY(file_storage_->saveKeyPair(
          key_type, kp.public_key, seed.unsafeBytes()));
      return keys_[key_type][kp.public_key];
    }

    outcome::result<OptRef<Keypair>> findKeypair(
        KeyType key_type, const PublicKey &pk) const override {
      if (auto it = keys_[key_type].find(pk); it != keys_[key_type].end()) {
        return *it;
      }
      return std::nullopt;
    }

    // outcome::result<OptRef<Keypair>> findKeypair(
    //     KeyType key_type, const filesystem::path &path) const override {
    //   SecureBuffer<> contents;
    //   if (not readFile(contents, path.string())) {
    //     return std::nullopt;
    //   }
    //   BOOST_ASSERT(Seed::size() == contents.size()
    //                or 2 * Seed::size() == contents.size());  // hex
    //   auto seed_res = [&]() -> outcome::result<Seed> {
    //     if (Seed::size() == contents.size()) {
    //       OUTCOME_TRY(_seed, Seed::from(std::move(contents)));
    //       return _seed;
    //     } else if (2 * Seed::size() == contents.size()) {  // hex-encoded
    //       std::span<char> char_content{
    //           reinterpret_cast<char *>(contents.data()), contents.size()};
    //       OUTCOME_TRY(_seed, Seed::fromHex(SecureCleanGuard{char_content}));
    //       return _seed;
    //     } else {
    //       return KeyStoreError::UNSUPPORTED_CRYPTO_TYPE;
    //     }
    //   }();
    //   OUTCOME_TRY(seed, seed_res);
    //   OUTCOME_TRY(kp, suite_->generateKeypair(seed, {}));
    //   return kp;
    // }

    outcome::result<std::vector<PublicKey>> getPublicKeys(
        KeyType key_type) const override {
      std::vector<PublicKey> res;
      res.reserve(keys_[key_type].size());
      for (auto &[public_key, private_key] : keys_[key_type]) {
        res.push_back(public_key);
      }
      return res;
    }

   private:
    template <typename CryptoSuite>
    outcome::result<typename CryptoSuite::Keypair> generateKeypair(
        std::string_view mnemonic_phrase, const CryptoSuite &suite) {
          
    }

    std::shared_ptr<T> suite_;
    std::shared_ptr<KeyFileStorage> file_storage_;
    std::shared_ptr<Bip39Provider> bip39_provider_;
    std::shared_ptr<CSPRNG> csprng_;
    std::unordered_map<KeyType, std::unordered_map<PublicKey, PrivateKey>>
        keys_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto
