/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_type.hpp"

#include <unordered_set>

#include <boost/endian/arithmetic.hpp>

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
        KEY_TYPE_LP2P,
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
              % std::alignment_of_v<KeyTypeId> == 0) {
        res = *reinterpret_cast<const kagome::crypto::KeyTypeId *>(str.data());
      } else {
        memcpy(&res, str.data(), sizeof(KeyTypeId));
      }
    }

    return res;
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
