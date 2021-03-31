/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_file_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, KeyFileStorage::Error, e) {
  using E = kagome::crypto::KeyFileStorage::Error;
  switch (e) {
    case E::WRONG_KEYFILE_NAME:
      return "specified file name is not a valid key file";
    case E::NOT_REGULAR_FILE:
      return "provided key file is not regular";
    case E::FAILED_OPEN_FILE:
      return "failed to open key file for reading";
    case E::FILE_DOESNT_EXIST:
      return "key file doesn't exist";
    case E::INVALID_FILE_FORMAT:
      return "specified key file is invalid";
    case E::INCONSISTENT_KEYFILE:
      return "key file is inconsistent, public key != derived public key";
    case E::KEYS_PATH_IS_NOT_DIRECTORY:
      return "specified key storage directory path is not a directory";
    case E::FAILED_CREATE_KEYS_DIRECTORY:
      return "failed to create key storage directory";
  }
  return "unknown KeyFileStorage error";
}

namespace {
  template <class T,
            typename = std::enable_if<std::is_integral_v<T> and sizeof(T) == 1>>
  kagome::crypto::KeyTypeId keyTypeFromBytes(gsl::span<const T> bytes) {
    BOOST_ASSERT_MSG(bytes.size() == 4, "Wrong span size");

    kagome::crypto::KeyTypeId res = static_cast<uint32_t>(bytes[3])
                                    + (static_cast<uint32_t>(bytes[2]) << 8)
                                    + (static_cast<uint32_t>(bytes[1]) << 16)
                                    + (static_cast<uint32_t>(bytes[0]) << 24);

    return res;
  }

}  // namespace

namespace kagome::crypto {

  using common::Buffer;

  outcome::result<std::unique_ptr<KeyFileStorage>> KeyFileStorage::createAt(
      Path keystore_path) {
    std::unique_ptr<KeyFileStorage> kfs{new KeyFileStorage(keystore_path)};
    OUTCOME_TRY(kfs->initialize());
    return kfs;
  }

  KeyFileStorage::KeyFileStorage(Path keystore_path)
      : keystore_path_{std::move(keystore_path)},
        logger_{log::createLogger("KeyFileStorage", "crypto_store")} {}

  outcome::result<std::pair<KeyTypeId, Buffer>>
  KeyFileStorage::parseKeyFileName(std::string_view file_name) const {
    if (file_name.size() < 4) {
      return Error::WRONG_KEYFILE_NAME;
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

    OUTCOME_TRY(public_key, Buffer::fromHex(public_key_hex));

    return {key_type, public_key};
  }

  KeyFileStorage::Path KeyFileStorage::composeKeyPath(
      KeyTypeId key_type, gsl::span<const uint8_t> public_key) const {
    auto &&key_type_str = decodeKeyTypeId(key_type);
    auto &&public_key_hex = common::hex_lower(public_key);

    return keystore_path_ / (key_type_str + public_key_hex);
  }

  outcome::result<void> KeyFileStorage::saveKeypair(
      KeyTypeId type,
      gsl::span<const uint8_t> public_key,
      gsl::span<const uint8_t> seed) const {
    std::ofstream file;

    auto &&path = composeKeyPath(type, public_key);
    file.open(path.native(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      return Error::FAILED_OPEN_FILE;
    }
    auto hex = common::hex_lower(seed);
    logger_->trace("Saving keypair (public: {}, secret: {}) to {}",
                   common::hex_lower(public_key),
                   hex,
                   path.native());
    file << hex;

    return outcome::success();
  }

  outcome::result<void> KeyFileStorage::initialize() {
    boost::system::error_code ec{};
    bool does_exist = boost::filesystem::exists(keystore_path_, ec);
    if (ec and ec != boost::system::errc::no_such_file_or_directory) {
      logger_->error("Error initializing key storage: {}", ec.message());
      return outcome::failure(ec);
    }
    if (does_exist) {
      // check whether specified path is a directory
      if (not boost::filesystem::is_directory(keystore_path_, ec)) {
        return Error::KEYS_PATH_IS_NOT_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error scanning key storage: {}", ec.message());
        return outcome::failure(ec);
      }
    } else {
      // try create directory
      if (not boost::filesystem::create_directories(keystore_path_, ec)) {
        return Error::FAILED_CREATE_KEYS_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error creating keystore dir: {}", ec.message());
        return outcome::failure(ec);
      }
    }

    return outcome::success();
  }

  outcome::result<std::string> KeyFileStorage::loadFileContent(
      const Path &file_path) const {
    if (!boost::filesystem::exists(file_path)) {
      return Error::FILE_DOESNT_EXIST;
    }

    std::ifstream file;

    file.open(file_path.string(), std::ios::in);
    if (!file.is_open()) {
      return Error::FAILED_OPEN_FILE;
    }

    std::string content;
    file >> content;
    logger_->trace("Loaded seed {} from {}", content, file_path.native());
    return content;
  }

  outcome::result<std::vector<Buffer>> KeyFileStorage::collectPublicKeys(
      KeyTypeId type) const {
    namespace fs = boost::filesystem;

    boost::system::error_code ec{};

    std::vector<Buffer> keys;

    fs::directory_iterator it{keystore_path_, ec}, end{};
    if (ec) {
      logger_->error("Error scanning keystore: {}", ec.message());
      return Error::FAILED_OPEN_FILE;
    }
    for (; it != end; ++it) {
      if (!fs::is_regular_file(*it)) {
        continue;
      }
      auto info = parseKeyFileName(it->path().filename().string());
      if (!info) {
        continue;
      }
      auto &[id, pk] = info.value();

      if (id == type) {
        keys.push_back(pk);
      }
    }
    return keys;
  }

  outcome::result<boost::optional<Buffer>> KeyFileStorage::searchForSeed(
      KeyTypeId type, gsl::span<const uint8_t> public_key_bytes) const {
    auto key_path = composeKeyPath(type, public_key_bytes);
    namespace fs = boost::filesystem;
    boost::system::error_code ec{};

    if (not fs::exists(key_path, ec)) {
      return boost::none;
    }
    OUTCOME_TRY(content, loadFileContent(key_path));
    OUTCOME_TRY(seed_bytes, common::unhex(content));
    return Buffer{std::move(seed_bytes)};
  }

}  // namespace kagome::crypto
