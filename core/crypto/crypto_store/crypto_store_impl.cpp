/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include <fstream>

#include <gsl/span>
#include "common/visitor.hpp"

namespace kagome::crypto {
  using DirectoryIterator = std::filesystem::directory_iterator;
  using CryptoTypeId = common::Blob<32>;

  static auto kEd25519 = CryptoTypeId::fromString("ed25").value();
  static auto kSr25519 = CryptoTypeId::fromString("sr25").value();
  static auto kSecp256k1 = CryptoTypeId::fromString("ecds").value();

  struct KeyFileInfo {
    KeyTypeId key_type_id;
    CryptoTypeId crypto_type_id;
    PublicKey public_key;
  };

  using MnemonicPhrase = std::string;
  using KeyFileContent = boost::variant<Seed, MnemonicPhrase>;

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
      if (file_name.size() < 8) {
        return CryptoStoreError::WRONG_KEYFILE_NAME;
      }

      auto key_type_str = file_name.substr(0, 4);
      OUTCOME_TRY(key_type,
                  keyTypeFromBytes(gsl::make_span(key_type_str.begin(),
                                                  key_type_str.end())));
      if (!isSupportedKeyType(key_type)) {
        return CryptoStoreError::UNSUPPORTED_KEY_TYPE;
      }

      auto crypto_type_str = file_name.substr(4, 4);
      OUTCOME_TRY(crypto_type, CryptoTypeId::fromString(crypto_type_str));
      auto public_key_hex = file_name.substr(8);

      PublicKey public_key;

      if (crypto_type == kEd25519) {
        OUTCOME_TRY(ed_public_bytes, ED25519PublicKey::fromHex(public_key_hex));
        public_key = ed_public_bytes;
      } else if (crypto_type == kSr25519) {
        OUTCOME_TRY(sr_public_bytes, SR25519PublicKey::fromHex(public_key_hex));
        public_key = sr_public_bytes;
      }
      // kSecp256k1 is not supported yet

      return KeyFileInfo{key_type, crypto_type, public_key};
    }

    outcome::result<std::string>

  }  // namespace

  CryptoStoreImpl::CryptoStoreImpl(
      std::filesystem::path store_path,
      std::shared_ptr<ED25519Provider> ed25519_provider,
      std::shared_ptr<SR25519Provider> sr25519_provider,
      std::shared_ptr<Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<Bip39Provider> bip39_provider)
      : keys_directory_(std::move(store_path)),
        ed25519_provider_(std::move(ed25519_provider_)),
        sr25519_provider_(std::move(sr25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        bip39_provider_(std::move(bip39_provider)) {
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
  }

  // TODO (yuraz): PRE-*** ecdsa will be added later
  outcome::result<CryptoStoreKeypair> CryptoStoreImpl::loadKeypair(
      const std::filesystem::path &key_path) {
    OUTCOME_TRY(info, parseKeyFileName(key_path.filename().string()));
    std::ifstream file;
    file.open(key_path, std::ios::in);
    if (!file.is_open()) {
      return CryptoStoreError::FAILED_OPEN_FILE;
    }

    std::string content;
    file >> content;
    OUTCOME_TRY(seed, Seed::fromString(content));
    if (info.crypto_type_id == kEd25519) {
      OUTCOME_TRY(kp, ed25519_provider_->generateKeyPair(seed));
      if (!kagome::visit_in_place(
              info.public_key,
              [&kp](const auto &v) -> bool { return kp.public_key == v; })) {
        return CryptoStoreError::INCONSISTENT_KEYFILE;
      }

      return CryptoStoreKeypair{kp, info.key_type_id};
    } else if (info.crypto_type_id == kSr25519) {
      OUTCOME_TRY(sr_kp, sr25519_provider_->generateKeypair(seed));
      if (kagome::visit_in_place(info.public_key,
                                 [&sr_kp](const auto &v) -> bool {
                                   return sr_kp.public_key == v;
                                 })) {
        return CryptoStoreError::INCONSISTENT_KEYFILE;
      }
    }
  }

  outcome::result<void> CryptoStoreImpl::initialize(
      std::filesystem::path keys_directory) {
    if (!std::filesystem::is_directory(keys_directory)) {
      return CryptoStoreError::KEYS_PATH_IS_NOT_DIRECTORY;
    }

    for (const auto &path : DirectoryIterator(keys_directory)) {
      if (!path.is_regular_file()) {
        return CryptoStoreError::NOT_REGULAR_FILE;
      }
      OUTCOME_TRY(pk_info, parseKeyFileName(std::string(path.path())));
      OUTCOME_TRY(pair, loadKeypair(path));
      switch (pk_info.key_type_id) {
        case supported_key_types::kAudi:

            ;
      }
      auto &&res = kagome::visit_in_place(
          key_pair,
          [this, &pk_info](const ED25519Keypair &kp) -> outcome::result<void> {
            ed_keys_.insert({})
          },
          [this, &pk_info](const SR25519Keypair &kp) -> outcome::result<void> {

          });
      if (!res) {
        return res;
      }
    }

    // load keys
    BOOST_ASSERT_MSG(false, "not implemented yet");
    return outcome::success();
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const ED25519Seed &seed) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const SR25519Seed &seed) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }

  outcome::result<ED25519Keypair> CryptoStoreImpl::generateEd25519KeyPair(
      KeyTypeId key_type) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
  }
  outcome::result<SR25519Keypair> CryptoStoreImpl::generateSr25519KeyPair(
      KeyTypeId key_type) {
    BOOST_ASSERT_MSG(false, "not implemented yet");
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
  }
  return "Unknown CryptoStoreError code";
}
