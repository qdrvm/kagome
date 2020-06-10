/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_type.hpp"

#include <unordered_set>

#include <boost/endian/arithmetic.hpp>

namespace kagome::crypto {

  bool isSupportedKeyType(KeyTypeId k) {
    using Type = SupportedKeyTypes;
    static const std::unordered_set<KeyTypeId> supported_types(
        {static_cast<uint32_t>(Type::BABE),
         static_cast<uint32_t>(Type::GRAN),
         static_cast<uint32_t>(Type::ACCO),
         static_cast<uint32_t>(Type::IMON),
         static_cast<uint32_t>(Type::AUDI)});

    return supported_types.find(k) != supported_types.end();
  }

  std::string decodeKeyTypeId(KeyTypeId param) {
    constexpr unsigned size = sizeof(KeyTypeId);
    constexpr unsigned bits = size * 8u;
    std::array<char, size> key_type{};

    boost::endian::endian_buffer<boost::endian::order::big, KeyTypeId, bits>
        buf{};
    buf = param;  // cannot initialize, only assign
    for (size_t i = 0; i < size; ++i) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      key_type.at(i) = buf.data()[i];
    }

    return std::string(key_type.begin(), key_type.end());
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
