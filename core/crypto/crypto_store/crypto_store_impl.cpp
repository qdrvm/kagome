/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include <fstream>

#include <gsl/span>
#include "common/visitor.hpp"

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
    case E::WRONG_SEED_SIZE:
      return "wrong seed size";
    case E::KEY_NOT_FOUND:
      return "key not found";
    case E::KEYS_PATH_IS_NOT_DIRECTORY:
      return "specified path is not a directory";
    case E::FAILED_CREATE_KEYS_DIRECTORY:
      return "failed to create directory";
  }
  return "Unknown CryptoStoreError code";
}

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

  }  // namespace

  outcome::result<std::pair<KeyTypeId, store::PublicKey>>
  CryptoStoreImpl::parseKeyFileName(std::string_view file_name) const {
    if (file_name.size() < 4 + store::PublicKey::size()) {
      return CryptoStoreError::WRONG_KEYFILE_NAME;
    }

    auto key_type_str = file_name.substr(0, 4);
    auto key_type = keyTypeFromBytes(
        gsl::make_span(key_type_str.begin(), key_type_str.end()));
    if (!isSupportedKeyType(key_type)) {
      logger_->warn("key type <ascii: {}, hex: {}> is not officially supported",
                    key_type_str,
                    kagome::common::hex_lower(gsl::make_span(
                        reinterpret_cast<const uint8_t *>(key_type_str.data()),
                        key_type_str.size())));
    }
    auto public_key_hex = file_name.substr(4);

    OUTCOME_TRY(public_key, store::PublicKey::fromHex(public_key_hex));

    return {key_type, public_key};
  }

  CryptoStoreImpl::Path CryptoStoreImpl::composeKeyPath(
      KeyTypeId key_type, const store::PublicKey &public_key) const {
    auto dir = keys_directory_;
    auto &&key_type_str = decodeKeyTypeId(key_type);
    auto &&public_key_hex = public_key.toHex();

    return dir / (key_type_str + public_key_hex);
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

  CryptoStoreImpl::CryptoStoreImpl(
      std::shared_ptr<Ed25519Provider> ed25519_provider,
      std::shared_ptr<Sr25519Provider> sr25519_provider,
      std::shared_ptr<Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<Bip39Provider> bip39_provider,
      std::shared_ptr<CSPRNG> random_generator)
      : ed25519_provider_(std::move(ed25519_provider)),
        sr25519_provider_(std::move(sr25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        bip39_provider_(std::move(bip39_provider)),
        random_generator_(std::move(random_generator)),
        logger_(common::createLogger("Crypto Store")) {
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
    BOOST_ASSERT(random_generator_ != nullptr);
  }

  outcome::result<void> CryptoStoreImpl::initialize(Path keys_directory) {
    boost::system::error_code ec {};
    bool does_exist = boost::filesystem::exists(keys_directory, ec);
    if (not ec) {
      logger_->error("Error initializing crypto store: {}", ec.message());
      return outcome::failure(ec);
    }
    if (does_exist) {
      // check whether specified path is a directory
      if (not boost::filesystem::is_directory(keys_directory, ec)) {
        return CryptoStoreError::KEYS_PATH_IS_NOT_DIRECTORY;
      }
      if (not ec) {
        logger_->error("Error initializing crypto store: {}", ec.message());
        return outcome::failure(ec);
      }
    } else {
      // try create directory
      if (not boost::filesystem::create_directory(keys_directory, ec)) {
        return CryptoStoreError::FAILED_CREATE_KEYS_DIRECTORY;
      }
      if (not ec) {
        logger_->error("Error initializing crypto store: {}", ec.message());
        return outcome::failure(ec);
      }
    }
    keys_directory_ = std::move(keys_directory);

    return outcome::success();
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    return generateKeypair<Ed25519Keypair, Ed25519Seed>(
        key_type, mnemonic_phrase, ed25519_provider_, ed_keys_);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    return generateKeypair<Sr25519Keypair, Sr25519Seed>(
        key_type, mnemonic_phrase, sr25519_provider_, sr_keys_);
  }

  Ed25519Keypair CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const Ed25519Seed &seed) {
    auto pair = ed25519_provider_->generateKeypair(seed);
    ed_keys_[key_type].emplace(pair.public_key, pair.secret_key);
    return pair;
  }

  Sr25519Keypair CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const Sr25519Seed &seed) {
    auto &&pair = sr25519_provider_->generateKeypair(seed);
    sr_keys_[key_type].emplace(pair.public_key, pair.secret_key);
    return pair;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519KeypairOnDisk(
      KeyTypeId key_type) {
    Ed25519Seed seed;
    auto &&bytes = random_generator_->randomBytes(Ed25519Seed::size());
    std::copy_n(bytes.begin(), Ed25519Seed::size(), seed.begin());

    auto pair = ed25519_provider_->generateKeypair(seed);
    OUTCOME_TRY(storeKeyfile(key_type, pair.public_key, seed));

    return pair;
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519KeypairOnDisk(
      KeyTypeId key_type) {
    Sr25519Seed seed;
    auto &&bytes = random_generator_->randomBytes(Sr25519Seed::size());
    std::copy_n(bytes.begin(), Sr25519Seed::size(), seed.begin());

    auto &&pair = sr25519_provider_->generateKeypair(seed);
    OUTCOME_TRY(storeKeyfile(key_type, pair.public_key, seed));

    return pair;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::findEd25519Keypair(
      KeyTypeId key_type, const Ed25519PublicKey &pk) const {
    // try find in memory
    if (ed_keys_.count(key_type) > 0) {
      const auto &map = ed_keys_.at(key_type);
      if (auto it = map.find(pk); it != map.end()) {
        return Ed25519Keypair{it->second, it->first};
      }
    }
    // try find in filesystem
    auto path = composeKeyPath(key_type, pk);
    if (!boost::filesystem::exists(path)) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(content, loadFileContent(path));
    OUTCOME_TRY(seed, Ed25519Seed::fromHex(content));

    return ed25519_provider_->generateKeypair(seed);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::findSr25519Keypair(
      KeyTypeId key_type, const Sr25519PublicKey &pk) const {
    // try find in memory
    if (sr_keys_.count(key_type) > 0) {
      const auto &map = sr_keys_.at(key_type);
      if (auto it = map.find(pk); it != map.end()) {
        return Sr25519Keypair{it->second, it->first};
      }
    }
    // try find in filesystem
    auto path = composeKeyPath(key_type, pk);
    if (!boost::filesystem::exists(path)) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(content, loadFileContent(path));
    OUTCOME_TRY(seed, Ed25519Seed::fromHex(content));

    return sr25519_provider_->generateKeypair(seed);
  }

  CryptoStore::Ed25519Keys CryptoStoreImpl::getEd25519PublicKeys(
      KeyTypeId key_type) const {
    return getPublicKeys<Ed25519Seed>(key_type, ed_keys_, ed25519_provider_);
  }

  CryptoStore::Sr25519Keys CryptoStoreImpl::getSr25519PublicKeys(
      KeyTypeId key_type) const {
    return getPublicKeys<Sr25519Seed>(key_type, sr_keys_, sr25519_provider_);
  }

  outcome::result<std::string> CryptoStoreImpl::loadFileContent(
      const boost::filesystem::path &file_path) {
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

  void CryptoStoreImpl::loadKeys(
      KeyTypeId key_type,
      const std::function<void(std::string_view,
                               std::string_view,
                               const store::PublicKey &)> &on_loaded) const {
    namespace fs = boost::filesystem;

    for (fs::directory_iterator it{keys_directory_}, end{}; it != end; ++it) {
      if (!fs::is_regular_file(*it)) {
        continue;
      }
      auto info = parseKeyFileName(it->path().filename().string());
      if (!info) {
        continue;
      }
      auto &[id, pk] = info.value();
      if (id == key_type /*and found_keys.count(pk) == 0*/) {
        auto &&content = loadFileContent(it->path());
        if (!content) {
          logger_->error("failed to load keyfile {} : {}",
                         it->path().string(),
                         content.error().message());
          continue;
        }
        on_loaded(it->path().string(), content.value(), pk);
      }
    }
  }

}  // namespace kagome::crypto
