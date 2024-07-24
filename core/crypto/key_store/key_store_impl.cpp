/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_store_impl.hpp"

#include <span>

#include "common/bytestr.hpp"
#include "common/visitor.hpp"
#include "crypto/key_store.hpp"
#include "utils/read_file.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, KeyStoreError, e) {
  switch (e) {
    using enum kagome::crypto::KeyStoreError;
    case UNSUPPORTED_KEY_TYPE:
      return "Key type is not supported";
    case UNSUPPORTED_CRYPTO_TYPE:
      return "Cryptographic type is not supported";
    case WRONG_SEED_SIZE:
      return "Wrong seed size";
    case KEY_NOT_FOUND:
      return "Key not found";
    case BABE_ALREADY_EXIST:
      return "BABE key already exists";
    case GRAN_ALREADY_EXIST:
      return "GRAN key already exists";
    case AUDI_ALREADY_EXIST:
      return "AUDI key already exists";
    case WRONG_PUBLIC_KEY:
      return "Public key doesn't match seed";
    case FAILED_TO_OPEN_FILE:
      return "Failed to open the key file";
    case INVALID_FILE_FORMAT:
      return "The key file is not valid "
             "(should be a BIP39 phrase or a hex-encoded seed)";
  }
  return "Unknown KeyStoreError code";
}

namespace kagome::crypto {

  libp2p::crypto::KeyPair ed25519KeyToLibp2pKeypair(const Ed25519Keypair &kp) {
    const auto &secret_key = kp.secret_key;
    const auto &public_key = kp.public_key;
    libp2p::crypto::PublicKey lp2p_public{
        {libp2p::crypto::Key::Type::Ed25519,
         std::vector<uint8_t>{public_key.cbegin(), public_key.cend()}}};
    libp2p::crypto::PrivateKey lp2p_private{
        {libp2p::crypto::Key::Type::Ed25519,
         std::vector<uint8_t>{secret_key.unsafeBytes().begin(),
                              secret_key.unsafeBytes().end()}}};
    return libp2p::crypto::KeyPair{
        .publicKey = lp2p_public,
        .privateKey = lp2p_private,
    };
  }

  KeyStore::KeyStore(
      std::unique_ptr<KeySuiteStore<Sr25519Provider>> sr25519,
      std::unique_ptr<KeySuiteStore<Ed25519Provider>> ed25519,
      std::unique_ptr<KeySuiteStore<EcdsaProvider>> ecdsa,
      std::unique_ptr<KeySuiteStore<BandersnatchProvider>> bandersnatch,
      std::shared_ptr<Ed25519Provider> ed25519_provider,
      std::shared_ptr<application::AppStateManager> app_manager,
      Config config)
      : config_{std::move(config)},
        sr25519_{std::move(sr25519)},
        ed25519_{std::move(ed25519)},
        ecdsa_{std::move(ecdsa)},
        bandersnatch_{std::move(bandersnatch)},
        ed25519_provider_{std::move(ed25519_provider)},
        app_manager_{std::move(app_manager)},
        logger_{log::createLogger("KeyStore", "crypto")} {
    BOOST_ASSERT(sr25519_);
    BOOST_ASSERT(ed25519_);
    BOOST_ASSERT(ecdsa_);
    BOOST_ASSERT(bandersnatch_);
    BOOST_ASSERT(ed25519_provider_);
    BOOST_ASSERT(app_manager_);
    app_manager_->takeControl(*this);
  }

  bool KeyStore::prepare() {
    if (auto res = scanKeyDirectory(config_.key_store_dir); !res) {
      SL_ERROR(
          logger_, "Failed to fetch keys from filesystem: {}", res.error());
      return false;
    }
    return true;
  }

  outcome::result<libp2p::crypto::KeyPair> KeyStore::loadLibp2pKeypair(
      const std::filesystem::path &file_path) const {
    SecureString content;
    if (!readFile(content, file_path)) {
      return KeyStoreError::KEY_NOT_FOUND;
    }
    Ed25519Seed seed;
    if (ED25519_SEED_LENGTH == content.size()) {
      OUTCOME_TRY(_seed,
                  Ed25519Seed::from(
                      SecureCleanGuard(str2byte(std::span<char>(content)))));
      seed = _seed;
    } else if (2 * ED25519_SEED_LENGTH == content.size()) {  // hex-encoded
      OUTCOME_TRY(_seed, Ed25519Seed::fromHex(SecureCleanGuard(content)));
      seed = _seed;
    } else {
      return KeyStoreError::UNSUPPORTED_CRYPTO_TYPE;
    }
    OUTCOME_TRY(kp, ed25519_provider_->generateKeypair(seed, {}));
    return ed25519KeyToLibp2pKeypair(kp);
  }

  outcome::result<KeyStore::SecureString> KeyStore::readSeed(
      const std::filesystem::path &file_path) const {
    SecureString content;
    if (!readFile(content, file_path)) {
      return KeyStoreError::FAILED_TO_OPEN_FILE;
    }
    if (not content.empty() and content[0] == '"') {
      if (auto str = jsonUnquote<SecureString>(content)) {
        content = std::move(*str);
      } else {
        return KeyStoreError::INVALID_FILE_FORMAT;
      }
    } else {
      // check it's a valid hex string
      OUTCOME_TRY(common::unhexWith0x(content));
    }
    return content;
  }

  outcome::result<void> KeyStore::scanKeyDirectory(
      const std::filesystem::path &dir) {
    // scan directory and collect every type-public key-seed triplet
    // for every key type
    //  for every allowed algorithm for this key type
    //    try to make a public key from the seed, if it matches the original
    //    key, add this keypair to the store
    std::error_code ec{};
    std::filesystem::directory_iterator iter{dir, ec};
    if (ec) {
      return ec;
    }
    for (auto &file : iter) {
      if (file.is_regular_file()) {
        OUTCOME_TRY(content, readSeed(file.path()));
        OUTCOME_TRY(decoded_parts,
                    decodeKeyFileName(file.path().filename().string()));
        auto [key_type, public_key] = std::move(decoded_parts);

        std::ignore = sr25519_->generateKeypair(key_type, content);
        std::ignore = ed25519_->generateKeypair(key_type, content);
        std::ignore = ecdsa_->generateKeypair(key_type, content);
        std::ignore = bandersnatch_->generateKeypair(key_type, content);
      }
    }
    return outcome::success();
  }
}  // namespace kagome::crypto
