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
  using DirectoryIterator = std::filesystem::directory_iterator;

  struct KeyFileInfo {
    KeyTypeId key_type_id;
    store::PublicKey public_key;
  };

  using MnemonicPhrase = std::string;
  using KeyFileContent = boost::variant<store::Seed, MnemonicPhrase>;

  namespace {
    template <class T>
    outcome::result<KeyTypeId> keyTypeFromBytes(gsl::span<const T> bytes) {
      if (bytes.size() != 4) {
        return CryptoStoreError::UNSUPPORTED_CRYPTO_TYPE;
      }

      KeyTypeId res = static_cast<uint32_t>(bytes[3])
                      + (static_cast<uint32_t>(bytes[2]) << 8)
                      + (static_cast<uint32_t>(bytes[1]) << 16)
                      + (static_cast<uint32_t>(bytes[0]) << 24);

      return res;
    }

    outcome::result<KeyFileInfo> parseKeyFileName(std::string_view file_name) {
      if (file_name.size() < 5) {
        return CryptoStoreError::WRONG_KEYFILE_NAME;
      }

      auto key_type_str = file_name.substr(0, 4);
      OUTCOME_TRY(key_type,
                  keyTypeFromBytes(gsl::make_span(key_type_str.begin(),
                                                  key_type_str.end())));
      if (!isSupportedKeyType(key_type)) {
        return CryptoStoreError::UNSUPPORTED_KEY_TYPE;
      }
      auto public_key_hex = file_name.substr(4);

      OUTCOME_TRY(public_key, store::PublicKey::fromHex(public_key_hex));

      return KeyFileInfo{key_type, public_key};
    }

    outcome::result<std::string> loadFile(const std::filesystem::path &p) {
      std::ifstream file;
      auto close_file = gsl::finally([&file] {
        if (file.is_open()) file.close();
      });

      file.open(p, std::ios::in);
      if (!file.is_open()) {
        return CryptoStoreError::FAILED_OPEN_FILE;
      }

      std::string content;
      file >> content;
      return content;
    }

    std::string composeKeyFileName(KeyTypeId key_type,
                                   const store::PublicKey &public_key) {
      auto &&key_type_str = decodeKeyTypeId(key_type);
      auto &&public_key_hex = public_key.toHex();
      return key_type_str + public_key_hex;
    }

    outcome::result<void> storeKeyfile(const std::filesystem::path &directory,
                                       KeyTypeId key_type,
                                       const store::PublicKey &public_key,
                                       const store::Seed &seed) {
      std::ofstream file;
      auto close_file = gsl::finally([&file] {
        if (file.is_open()) file.close();
      });

      auto dir = directory;
      auto &&file_name = composeKeyFileName(key_type, public_key);
      auto &&path = dir.append(file_name);
      file.open(path, std::ios::out | std::ios::trunc);
      if (!file.is_open()) {
        return CryptoStoreError::FAILED_OPEN_FILE;
      }

      file << seed.toHex();
      file.flush();

      return outcome::success();
    }
  }  // namespace

  CryptoStoreImpl::CryptoStoreImpl(
      std::shared_ptr<ED25519Provider> ed25519_provider,
      std::shared_ptr<SR25519Provider> sr25519_provider,
      std::shared_ptr<Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<Bip39Provider> bip39_provider)
      : keys_directory_{},
        ed25519_provider_(std::move(ed25519_provider)),
        sr25519_provider_(std::move(sr25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        bip39_provider_(std::move(bip39_provider)) {
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
  }

  outcome::result<store::KeyPair> CryptoStoreImpl::loadKeypair(
      store::CryptoId crypto_id, const std::filesystem::path &key_path) {
    OUTCOME_TRY(info, parseKeyFileName(key_path.filename().string()));
    OUTCOME_TRY(content, loadFile(key_path));
    OUTCOME_TRY(seed, store::Seed::fromString(content));

    switch (crypto_id) {
      case store::CryptoId::ED25519: {
        OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(seed));
        return pair;
      }
      case store::CryptoId::SR25519: {
        return sr25519_provider_->generateKeypair(seed);
      }
      case store::CryptoId::SECP256k1:
      default:
        break;
    }

    return CryptoStoreError::UNSUPPORTED_CRYPTO_TYPE;
  }

  outcome::result<void> CryptoStoreImpl::initialize(
      std::filesystem::path keys_directory) {
    if (!std::filesystem::is_directory(keys_directory)) {
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

    ed_keys_[key_type].insert({pair.public_key, pair.private_key});

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

    sr_keys_[key_type].insert({pair.public_key, pair.secret_key});

    return pair;
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const ED25519Seed &seed) {
    OUTCOME_TRY(pair, ed25519_provider_->generateKeypair(seed));
    ed_keys_[key_type].insert({pair.public_key, pair.private_key});
    return pair;
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const SR25519Seed &seed) {
    auto &&pair = sr25519_provider_->generateKeypair(seed);
    sr_keys_[key_type].insert({pair.public_key, pair.secret_key});
    return pair;
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519KeyPair(
      KeyTypeId key_type) {
    OUTCOME_TRY(pair, ed25519_provider_->generateKeypair());
    ed_keys_[key_type].insert({pair.public_key, pair.private_key});
//    OUTCOME_TRY(storeKeyfile(keys_directory_, key_type, pair.public_key, pair.private_key));
    return pair;
  }
  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519KeyPair(
      KeyTypeId key_type) {
    auto &&pair = sr25519_provider_->generateKeypair();
    sr_keys_[key_type].insert({pair.public_key, pair.secret_key});
//    OUTCOME_TRY(storeKeyfile(keys_directory_, key_type, pair.public_key, pair.secret_key));
    return pair;
  }

  boost::optional<ED25519Keypair> CryptoStoreImpl::findEd25519Keypair(
      KeyTypeId key_type, const ED25519PublicKey &pk) const {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  boost::optional<SR25519Keypair> CryptoStoreImpl::findSr25519Keypair(
      KeyTypeId key_type, const SR25519PublicKey &pk) const {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  CryptoStore::ED25519Keys CryptoStoreImpl::getEd25519PublicKeys(
      KeyTypeId key_type) const {
    ED25519Keys keys;
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  CryptoStore::SR25519Keys CryptoStoreImpl::getSr25519PublicKeys(
      KeyTypeId key_type) const {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, CryptoStoreError, e) {
  using E = kagome::crypto::CryptoStoreError;
  switch (e) {
    case E::WRONG_KEYFILE_NAME:
      return "Specified file name is not a valid key file";
    case E::UNSUPPORTED_KEY_TYPE:
      return "Key type is not supported";
    case E::UNSUPPORTED_CRYPTO_TYPE:
      return "Cryptographic type is not supported";
    case E::NOT_REGULAR_FILE:
      return "Provided file is not regular";
    case E::FAILED_OPEN_FILE:
      return "failed to open file for reading";
    case E::INVALID_FILE_FORMAT:
      return "Specified key file is invalid";
    case E::INCONSISTENT_KEYFILE:
      return "Key file is inconsistent, public key != derived public key";
    case E::KEYS_PATH_IS_NOT_DIRECTORY:
      return "Specified path to key files is not a valid directory";
    case E::WRONG_SEED_SIZE:
      return "Wrong seed size";
  }
  return "Unknown CryptoStoreError code";
}
