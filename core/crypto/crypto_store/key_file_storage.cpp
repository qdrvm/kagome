/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_file_storage.hpp"

#include <fstream>

#include "common/hexutil.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "filesystem/common.hpp"
#include "utils/json_unquote.hpp"
#include "utils/read_file.hpp"

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
    OUTCOME_TRY(info, decodeKeyFileName(file_name));
    auto key_type_str = file_name.substr(0, 8);
    if (not KeyType::is_supported(info.first)) {
      auto ascii_res = common::unhex(key_type_str);
      if (ascii_res.has_value()) {
        std::string_view ascii(
            reinterpret_cast<const char *>(ascii_res.value().data()),  // NOLINT
            ascii_res.value().size());
        logger_->warn(
            "key type <ascii: '{}', hex: {:08x}> is not officially supported",
            ascii,
            (KeyTypeId)info.first);
      } else {
        logger_->warn("key type <hex: {:08x}> is not officially supported",
                      (KeyTypeId)info.first);
      }
    }
    return info;
  }

  KeyFileStorage::Path KeyFileStorage::composeKeyPath(
      KeyTypeId key_type, gsl::span<const uint8_t> public_key) const {
    return keystore_path_ / encodeKeyFileName(key_type, public_key);
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
    std::error_code ec{};
    bool does_exist = filesystem::exists(keystore_path_, ec);
    if (ec and ec != std::errc::no_such_file_or_directory) {
      logger_->error("Error initializing key storage: {}", ec);
      return outcome::failure(ec);
    }
    if (does_exist) {
      // check whether specified path is a directory
      if (not filesystem::is_directory(keystore_path_, ec)) {
        return Error::KEYS_PATH_IS_NOT_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error scanning key storage: {}", ec);
        return outcome::failure(ec);
      }
    } else {
      // try create directory
      if (not filesystem::create_directories(keystore_path_, ec)) {
        return Error::FAILED_CREATE_KEYS_DIRECTORY;
      }
      if (ec) {
        logger_->error("Error creating keystore dir: {}", ec);
        return outcome::failure(ec);
      }
    }

    return outcome::success();
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
    namespace fs = filesystem;

    std::error_code ec{};

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

  outcome::result<std::optional<std::string>> KeyFileStorage::searchForPhrase(
      KeyTypeId type, gsl::span<const uint8_t> public_key_bytes) const {
    auto key_path = composeKeyPath(type, public_key_bytes);
    namespace fs = filesystem;
    std::error_code ec{};

    if (not fs::exists(key_path, ec)) {
      return std::nullopt;
    }
    std::string content;
    if (not readFile(content, key_path.string())) {
      return std::nullopt;
    }
    if (not content.empty() and content[0] == '"') {
      if (auto str = jsonUnquote(content)) {
        return str;
      }
      return Error::INVALID_FILE_FORMAT;
    }
    OUTCOME_TRY(common::unhexWith0x(content));
    return content;
  }

}  // namespace kagome::crypto
