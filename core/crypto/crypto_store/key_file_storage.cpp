/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_file_storage.hpp"

#include "common/hexutil.hpp"
#include "crypto/crypto_store/key_type.hpp"

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
    auto key_type = decodeKeyTypeIdFromStr(key_type_str);
    if (!isSupportedKeyType(key_type)) {
      logger_->warn(
          "key type <ascii: {}, hex: {:08x}> is not officially supported",
          key_type_str,
          key_type);
    }
    auto public_key_hex = file_name.substr(4);

    OUTCOME_TRY(public_key, Buffer::fromHex(public_key_hex));

    return {key_type, public_key};
  }

  KeyFileStorage::Path KeyFileStorage::composeKeyPath(
      KeyTypeId key_type, gsl::span<const uint8_t> public_key) const {
    auto &&key_type_str = encodeKeyTypeIdToStr(key_type);
    auto &&public_key_hex = common::hex_lower(public_key);

    return keystore_path_ / (key_type_str + public_key_hex);
  }

  outcome::result<void> KeyFileStorage::saveKeyPair(
      KeyTypeId type,
      gsl::span<const uint8_t> public_key,
      gsl::span<const uint8_t> seed) const {
    auto &&path = composeKeyPath(type, public_key);
    OUTCOME_TRY(saveKeyHexAtPath(seed, path));
    SL_TRACE(logger_,
             "Saving keypair (public: {}) to {}",
             common::hex_lower(public_key),
             path.native());
    return outcome::success();
  }

  outcome::result<void> KeyFileStorage::initialize() {
    boost::system::error_code ec{};
    bool does_exist = boost::filesystem::exists(keystore_path_, ec);
    if (ec and ec != boost::system::errc::no_such_file_or_directory) {
      logger_->error("Error initializing key storage: {}", ec);
      return outcome::failure(ec);
    }
    if (does_exist) {
      // check whether specified path is a directory
      if (not boost::filesystem::is_directory(keystore_path_, ec)) {
        return Error::KEYS_PATH_IS_NOT_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error scanning key storage: {}", ec);
        return outcome::failure(ec);
      }
    } else {
      // try create directory
      if (not boost::filesystem::create_directories(keystore_path_, ec)) {
        return Error::FAILED_CREATE_KEYS_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error creating keystore dir: {}", ec);
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

    file.open(file_path.string(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
      return Error::FAILED_OPEN_FILE;
    }

    std::string content;
    file >> content;
    SL_TRACE(logger_, "Loaded seed {} from {}", content, file_path.native());
    return content;
  }

  outcome::result<void> KeyFileStorage::saveKeyHexAtPath(
      gsl::span<const uint8_t> private_key,
      const KeyFileStorage::Path &path) const {
    std::ofstream file;
    file.open(path.native(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      return Error::FAILED_OPEN_FILE;
    }
    auto hex = common::hex_lower_0x(private_key);
    file << hex;
    SL_TRACE(logger_, "Saving key to {}", path.native());
    return outcome::success();
  }

  outcome::result<std::vector<Buffer>> KeyFileStorage::collectPublicKeys(
      KeyTypeId type) const {
    namespace fs = boost::filesystem;

    boost::system::error_code ec{};

    std::vector<Buffer> keys;

    fs::directory_iterator it{keystore_path_, ec}, end{};
    if (ec) {
      logger_->error("Error scanning keystore: {}", ec);
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

  outcome::result<std::optional<Buffer>> KeyFileStorage::searchForSeed(
      KeyTypeId type, gsl::span<const uint8_t> public_key_bytes) const {
    auto key_path = composeKeyPath(type, public_key_bytes);
    namespace fs = boost::filesystem;
    boost::system::error_code ec{};

    if (not fs::exists(key_path, ec)) {
      return std::nullopt;
    }
    OUTCOME_TRY(content, loadFileContent(key_path));
    OUTCOME_TRY(seed_bytes, common::unhexWith0x(content));
    return Buffer{std::move(seed_bytes)};
  }

}  // namespace kagome::crypto
