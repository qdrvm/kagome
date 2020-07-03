/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include <fstream>

#include <gsl/span>
#include "common/visitor.hpp"
#include "crypto/bip39/mnemonic.hpp"

namespace kagome::crypto {
  using DirectoryIterator = boost::filesystem::directory_iterator;

  namespace {
    template <class T>
    KeyTypeId keyTypeFromBytes(gsl::span<const T> bytes) {
      BOOST_ASSERT_MSG(bytes.size() == 4, "Wrong span size");

      KeyTypeId res = static_cast<uint32_t>(bytes[3])
                      + (static_cast<uint32_t>(bytes[2]) << 8)
                      + (static_cast<uint32_t>(bytes[1]) << 16)
                      + (static_cast<uint32_t>(bytes[0]) << 24);

      return res;
    }

    outcome::result<std::pair<KeyTypeId, store::PublicKey>> parseKeyFileName(
        std::string_view file_name) {
      if (file_name.size() < 4 + store::PublicKey::size()) {
        return CryptoStoreError::WRONG_KEYFILE_NAME;
      }

      auto key_type_str = file_name.substr(0, 4);
      auto key_type = keyTypeFromBytes(
          gsl::make_span(key_type_str.begin(), key_type_str.end()));
      if (!isSupportedKeyType(key_type)) {
        return CryptoStoreError::UNSUPPORTED_KEY_TYPE;
      }
      auto public_key_hex = file_name.substr(4);

      OUTCOME_TRY(public_key, store::PublicKey::fromHex(public_key_hex));

      return {key_type, public_key};
    }
  }  // namespace

  CryptoStoreImpl::Path CryptoStoreImpl::composeKeyPath(
      KeyTypeId key_type, const store::PublicKey &public_key) const {
    auto dir = keys_directory_;
    auto &&key_type_str = decodeKeyTypeId(key_type);
    auto &&public_key_hex = public_key.toHex();

    return dir.append(key_type_str + public_key_hex);
  }

  outcome::result<void> CryptoStoreImpl::storeKeyfile(
      KeyTypeId key_type,
      const store::PublicKey &public_key,
      const store::Seed &seed) {
    std::ofstream file;

    auto dir = keys_directory_;
    auto &&path = composeKeyPath(key_type, public_key);
    file.open(path.string(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      return CryptoStoreError::FAILED_OPEN_FILE;
    }

    file << seed.toHex();

    return outcome::success();
  }

  outcome::result<std::string> CryptoStoreImpl::loadFile(
      const Path &file_path) const {
    if (!boost::filesystem::exists(file_path)) {
      return CryptoStoreError::FILE_DOESNT_EXIST;
    }

    std::ifstream file;
    auto close_file = gsl::finally([&file] {
      if (file.is_open()) file.close();
    });

    file.open(file_path.string(), std::ios::in);
    if (!file.is_open()) {
      return CryptoStoreError::FAILED_OPEN_FILE;
    }

    std::string content;
    file >> content;
    return content;
  }

  CryptoStoreImpl::CryptoStoreImpl(
      std::shared_ptr<ED25519Provider> ed25519_provider,
      std::shared_ptr<SR25519Provider> sr25519_provider,
      std::shared_ptr<Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<Bip39Provider> bip39_provider,
      std::shared_ptr<CSPRNG> random_generator)
      : keys_directory_{},
        ed25519_provider_(std::move(ed25519_provider)),
        sr25519_provider_(std::move(sr25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        bip39_provider_(std::move(bip39_provider)),
        random_generator_(std::move(random_generator)) {
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
    BOOST_ASSERT(random_generator_ != nullptr);
  }

  outcome::result<void> CryptoStoreImpl::initialize(Path keys_directory) {
    if (!boost::filesystem::exists(keys_directory)) {
      if (!boost::filesystem::create_directory(keys_directory)) {
        return CryptoStoreError::FAILED_CREATE_DIRECTORY;
      }
    }

    if (!boost::filesystem::is_directory(keys_directory)) {
      return CryptoStoreError::KEYS_PATH_IS_NOT_DIRECTORY;
    }

    keys_directory_ = std::move(keys_directory);

    return outcome::success();
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(mnemonic, bip39::Mnemonic::parse(mnemonic_phrase));
    OUTCOME_TRY(entropy, bip39_provider_->calculateEntropy(mnemonic.words));
    OUTCOME_TRY(seed, bip39_provider_->makeSeed(entropy, mnemonic.password));
    if (seed.size() < ED25519Seed::size()) {
      return CryptoStoreError::WRONG_SEED_SIZE;
    }

    OUTCOME_TRY(ed_seed,
                ED25519Seed::fromSpan(
                    gsl::make_span(seed).subspan(0, ED25519Seed::size())));
    OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(ed_seed));

    ed_keys_[key_type].emplace(pair.public_key, pair.private_key);

    return pair;
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(mnemonic, bip39::Mnemonic::parse(mnemonic_phrase));
    OUTCOME_TRY(entropy, bip39_provider_->calculateEntropy(mnemonic.words));
    OUTCOME_TRY(seed, bip39_provider_->makeSeed(entropy, mnemonic.password));
    if (seed.size() < SR25519Seed ::size()) {
      return CryptoStoreError::WRONG_SEED_SIZE;
    }

    OUTCOME_TRY(sr_seed,
                ED25519Seed::fromSpan(
                    gsl::make_span(seed).subspan(0, ED25519Seed::size())));
    auto &&pair = sr25519_provider_->generateKeypair(sr_seed);

    sr_keys_[key_type].emplace(pair.public_key, pair.secret_key);

    return pair;
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const ED25519Seed &seed) {
    OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(seed));
    ed_keys_[key_type].emplace(pair.public_key, pair.private_key);
    return pair;
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const SR25519Seed &seed) {
    auto &&pair = sr25519_provider_->generateKeypair(seed);
    sr_keys_[key_type].emplace(pair.public_key, pair.secret_key);
    return pair;
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type) {
    ED25519Seed seed;
    auto &&bytes = random_generator_->randomBytes(ED25519Seed::size());
    std::copy_n(bytes.begin(), ED25519Seed::size(), seed.begin());

    OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(seed));
    OUTCOME_TRY(storeKeyfile(key_type, pair.public_key, seed));

    return pair;
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type) {
    SR25519Seed seed;
    auto &&bytes = random_generator_->randomBytes(SR25519Seed::size());
    std::copy_n(bytes.begin(), SR25519Seed::size(), seed.begin());

    auto &&pair = sr25519_provider_->generateKeypair(seed);
    OUTCOME_TRY(storeKeyfile(key_type, pair.public_key, seed));

    return pair;
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::findEd25519Keypair(
      KeyTypeId key_type, const ED25519PublicKey &pk) const {
    // try find in memory
    if (ed_keys_.count(key_type) > 0) {
      const auto &map = ed_keys_.at(key_type);
      if (auto it = map.find(pk); it != map.end()) {
        return ED25519Keypair{it->second, it->first};
      }
    }
    // try find in filesystem
    auto path = composeKeyPath(key_type, pk);
    if (!boost::filesystem::exists(path)) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(content, loadFile(path));
    OUTCOME_TRY(seed, ED25519Seed::fromHex(content));

    return ed25519_provider_->generateKeypair(seed);
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::findSr25519Keypair(
      KeyTypeId key_type, const SR25519PublicKey &pk) const {
    // try find in memory
    if (sr_keys_.count(key_type) > 0) {
      const auto &map = sr_keys_.at(key_type);
      if (auto it = map.find(pk); it != map.end()) {
        return SR25519Keypair{it->second, it->first};
      }
    }
    // try find in filesystem
    auto path = composeKeyPath(key_type, pk);
    if (!boost::filesystem::exists(path)) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(content, loadFile(path));
    OUTCOME_TRY(seed, ED25519Seed::fromHex(content));

    return sr25519_provider_->generateKeypair(seed);
  }

  outcome::result<CryptoStore::ED25519Keys>
  CryptoStoreImpl::getEd25519PublicKeys(KeyTypeId key_type) const {
    std::set<ED25519PublicKey> keys;
    // iterate over in-memory map
    if (ed_keys_.find(key_type) != ed_keys_.end()) {
      const auto &map = ed_keys_.at(key_type);
      for (const auto &k : map) {
        keys.emplace(k.first);
      }
    }

    // iterate over fs
    for (DirectoryIterator it(keys_directory_), end{}; it != end; ++it) {
      if (!boost::filesystem::is_regular_file(*it)) {
        continue;
      }
      auto info = parseKeyFileName(it->path().filename().string());
      if (!info) {
        continue;
      }
      auto &[id, pk] = info.value();
      if (id == key_type && keys.count(pk) == 0) {
        OUTCOME_TRY(content, loadFile(it->path()));
        OUTCOME_TRY(seed, ED25519Seed::fromHex(content));
        OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(seed));
        if (pair.public_key == pk) {
          keys.emplace(pk);
        }
      }
    }

    return ED25519Keys(keys.begin(), keys.end());
  }

  outcome::result<CryptoStore::SR25519Keys>
  CryptoStoreImpl::getSr25519PublicKeys(KeyTypeId key_type) const {
    std::set<SR25519PublicKey> keys;
    // iterate over in-memory map
    if (sr_keys_.find(key_type) != sr_keys_.end()) {
      const auto &map = ed_keys_.at(key_type);
      for (const auto &k : map) {
        keys.emplace(k.first);
      }
    }

    // iterate over fs
    for (DirectoryIterator it(keys_directory_), end{}; it != end; ++it) {
      if (!boost::filesystem::is_regular_file(*it)) {
        continue;
      }
      auto info = parseKeyFileName(it->path().filename().string());
      if (!info) {
        continue;
      }
      auto &[id, pk] = info.value();
      if (id == key_type && keys.count(pk) == 0) {
        OUTCOME_TRY(content, loadFile(it->path()));
        OUTCOME_TRY(seed, SR25519Seed::fromHex(content));
        auto &&pair = sr25519_provider_->generateKeypair(seed);
        if (pair.public_key == pk) {
          keys.emplace(pk);
        }
      }
    }

    return SR25519Keys(keys.begin(), keys.end());
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, CryptoStoreError, e) {
  using E = kagome::crypto::CryptoStoreError;
  switch (e) {
    case E::WRONG_KEYFILE_NAME:
      return "specified file name is not a valid key file";
    case E::UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
    case E::UNSUPPORTED_CRYPTO_TYPE:
      return "cryptographic type is not supported";
    case E::NOT_REGULAR_FILE:
      return "provided file is not regular";
    case E::FAILED_OPEN_FILE:
      return "failed to open file for reading";
    case E::FILE_DOESNT_EXIST:
      return "file doesn't exist";
    case E::INVALID_FILE_FORMAT:
      return "specified key file is invalid";
    case E::INCONSISTENT_KEYFILE:
      return "key file is inconsistent, public key != derived public key";
    case E::FAILED_CREATE_DIRECTORY:
      return "failed to create directory";
    case E::KEYS_PATH_IS_NOT_DIRECTORY:
      return "specified path to key files is not a valid directory";
    case E::WRONG_SEED_SIZE:
      return "wrong seed size";
    case E::KEY_NOT_FOUND:
      return "key not found";
  }
  return "Unknown CryptoStoreError code";
}
