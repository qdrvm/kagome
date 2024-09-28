/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_file_storage.hpp"

#include "common/hexutil.hpp"
#include "crypto/key_store/key_type.hpp"
#include "filesystem/common.hpp"
#include "utils/json_unquote.hpp"
#include "utils/mkdirs.hpp"
#include "utils/read_file.hpp"
#include "utils/write_file.hpp"

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
  }
  return "unknown KeyFileStorage error";
}

namespace kagome::crypto {

  using common::Buffer;

  outcome::result<std::unique_ptr<KeyFileStorage>> KeyFileStorage::createAt(
      Path keystore_path) {
    std::unique_ptr<KeyFileStorage> kfs{
        new KeyFileStorage(std::move(keystore_path))};
    OUTCOME_TRY(kfs->initialize());
    return kfs;
  }

  KeyFileStorage::KeyFileStorage(Path keystore_path)
      : keystore_path_{std::move(keystore_path)},
        logger_{log::createLogger("KeyFileStorage", "key_store")} {}

  outcome::result<std::pair<KeyType, Buffer>> KeyFileStorage::parseKeyFileName(
      std::string_view file_name) const {
    OUTCOME_TRY(info, decodeKeyFileName(file_name));
    if (not KeyTypes::is_supported(info.first)) {
      logger_->warn("key type {} is not officially supported", info.first);
    }
    return info;
  }

  KeyFileStorage::Path KeyFileStorage::composeKeyPath(
      KeyType key_type, common::BufferView public_key) const {
    return keystore_path_ / encodeKeyFileName(key_type, public_key);
  }

  outcome::result<void> KeyFileStorage::saveKeyPair(
      KeyType type,
      common::BufferView public_key,
      common::BufferView seed) const {
    auto &&path = composeKeyPath(type, public_key);
    OUTCOME_TRY(saveKeyHexAtPath(seed, path));
    SL_TRACE(logger_,
             "Saving keypair (public: {}) to {}",
             common::hex_lower(public_key),
             path.native());
    return outcome::success();
  }

  outcome::result<void> KeyFileStorage::initialize() {
    OUTCOME_TRY(mkdirs(keystore_path_));
    return outcome::success();
  }

  outcome::result<void> KeyFileStorage::saveKeyHexAtPath(
      common::BufferView private_key, const KeyFileStorage::Path &path) const {
    OUTCOME_TRY(writeFile(path, common::hex_lower_0x(private_key)));
    SL_TRACE(logger_, "Saving key to {}", path.native());
    return outcome::success();
  }

  outcome::result<bool> KeyFileStorage::searchForKey(
      KeyType type, common::BufferView public_key_bytes) const {
    auto key_path = composeKeyPath(type, public_key_bytes);
    std::error_code ec{};

    if (not filesystem::exists(key_path, ec)) {
      return false;
    }
    std::string content;
    if (not readFile(content, key_path.string())) {
      return false;
    }
    if (not content.empty() and content[0] == '"') {
      if (auto str = jsonUnquote(content)) {
        return true;
      }
      return Error::INVALID_FILE_FORMAT;
    }
    OUTCOME_TRY(common::unhexWith0x(content));
    return true;
  }

}  // namespace kagome::crypto
