/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_type.hpp"

#include <unordered_set>

#include "common/blob.hpp"
#include "common/bytestr.hpp"

namespace kagome::crypto {

  bool KeyType::is_supported() const {
    return KeyTypes::is_supported(*this);
  }

  std::string encodeKeyTypeToStr(const KeyType &key_type) {
    const auto *p = reinterpret_cast<const char *>(&key_type);
    return {p, p + sizeof(uint32_t)};
  }

  KeyType decodeKeyTypeFromStr(std::string_view str) {
    uint32_t res = 0;

    if (str.size() == sizeof(uint32_t)) {
      // string's data is aligned as KeyType
      if (reinterpret_cast<uintptr_t>(str.data())
              % std::alignment_of_v<uint32_t>
          == 0) {
        res = *reinterpret_cast<const uint32_t *>(str.data());
      } else {
        memcpy(&res, str.data(), sizeof(uint32_t));
      }
    }

    return res;
  }

  std::string encodeKeyFileName(const KeyType &type, common::BufferView key) {
    return common::hex_lower(str2byte(encodeKeyTypeToStr(type))) + key.toHex();
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
    return std::make_pair(decodeKeyTypeFromStr(byte2str(type_raw)),
                          std::move(key));
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, KeyTypeError, e) {
  using Error = kagome::crypto::KeyTypeError;
  switch (e) {
    case Error::UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
    case Error::UNSUPPORTED_KEY_TYPE_ID:
      return "key type id is not supported";
  }
  return "Unknown KeyTypeError";
}
