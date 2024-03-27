/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_type.hpp"

#include <unordered_set>

#include "common/blob.hpp"
#include "common/bytestr.hpp"

namespace kagome::crypto {

  bool KeyType::is_supported() const {
    return KeyTypes::is_supported(*this);
  }

  std::string encodeKeyFileName(const KeyType &type, common::BufferView key) {
    return common::hex_lower(str2byte(type.toString())) + key.toHex();
  }

  outcome::result<std::pair<KeyType, common::Buffer>> decodeKeyFileName(
      std::string_view name) {
    std::string_view type_str = name;
    std::string_view key_str;
    if (name.size() > 8) {
      type_str = name.substr(0, 8);
      key_str = name.substr(8);
    }
    OUTCOME_TRY(type_raw, common::Blob<4>::fromHex(type_str));
    OUTCOME_TRY(key, common::Buffer::fromHex(key_str));
    if (auto key_type = KeyType::fromString(byte2str(type_raw))) {
      return std::make_pair(*key_type, std::move(key));
    }
    return KeyTypeError::UNSUPPORTED_KEY_TYPE;
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, KeyTypeError, e) {
  using Error = kagome::crypto::KeyTypeError;
  switch (e) {
    case Error::UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
  }
  return "Unknown KeyTypeError";
}
