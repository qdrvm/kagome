/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_type.hpp"

#include <unordered_set>

#include "common/blob.hpp"
#include "common/bytestr.hpp"

namespace kagome::crypto {

  bool isSupportedKeyType(KeyTypeId k) {
    static const std::unordered_set<KeyTypeId> supported_types = {
        KEY_TYPE_GRAN,
        KEY_TYPE_BABE,
        KEY_TYPE_IMON,
        KEY_TYPE_PARA,
        KEY_TYPE_ASGN,
        KEY_TYPE_AUDI,
        KEY_TYPE_ACCO,
        KEY_TYPE_BEEF,
    };

    return supported_types.count(k) > 0;
  }

  std::string encodeKeyTypeIdToStr(KeyTypeId key_type_id) {
    const auto *p = reinterpret_cast<const char *>(&key_type_id);
    std::string res(p, p + sizeof(KeyTypeId));
    return res;
  }

  KeyTypeId decodeKeyTypeIdFromStr(std::string_view str) {
    kagome::crypto::KeyTypeId res = 0;

    if (str.size() == sizeof(KeyTypeId)) {
      // string's data is aligned as KeyTypeId
      if (reinterpret_cast<uintptr_t>(str.data())
              % std::alignment_of_v<KeyTypeId>
          == 0) {
        res = *reinterpret_cast<const kagome::crypto::KeyTypeId *>(str.data());
      } else {
        memcpy(&res, str.data(), sizeof(KeyTypeId));
      }
    }

    return res;
  }

  std::string encodeKeyFileName(KeyTypeId type, common::BufferView key) {
    return common::hex_lower(str2byte(encodeKeyTypeIdToStr(type)))
         + key.toHex();
  }

  outcome::result<std::pair<KeyTypeId, common::Buffer>> decodeKeyFileName(
      std::string_view name) {
    std::string_view type_str = name;
    std::string_view key_str;
    if (name.size() > 8) {
      type_str = name.substr(0, 8);
      key_str = name.substr(8);
    }
    OUTCOME_TRY(type_raw, common::Blob<4>::fromHex(type_str));
    OUTCOME_TRY(key, common::Buffer::fromHex(key_str));
    return std::make_pair(decodeKeyTypeIdFromStr(byte2str(type_raw)),
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
