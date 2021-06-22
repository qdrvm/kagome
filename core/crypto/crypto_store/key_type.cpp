/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/key_type.hpp"

#include <unordered_set>
#include <array>

#include <boost/endian/arithmetic.hpp>

namespace kagome::crypto {

  bool isSupportedKeyType(KeyTypeId k) {
    static const std::unordered_set<KeyTypeId> supported_types = {
        KEY_TYPE_GRAN,
        KEY_TYPE_ACCO,
        KEY_TYPE_AUDI,
        KEY_TYPE_BABE,
        KEY_TYPE_IMON,
        KEY_TYPE_LP2P};

    return supported_types.count(k) > 0;
  }

  KeyTypeId encodeKeyTypeId(std::string str) {
    constexpr unsigned size = sizeof(KeyTypeId);
    if(str.size() != size) {
      return 0u;
    }
    // little endian order
    KeyTypeId res = (static_cast<uint32_t>(str[3]) & 0xFF)
                    | ((static_cast<uint32_t>(str[2]) & 0xFF) << 8)
                    | ((static_cast<uint32_t>(str[1]) & 0xFF) << 16)
                    | ((static_cast<uint32_t>(str[0]) & 0xFF) << 24);
    return res;
  }

  std::string decodeKeyTypeId(KeyTypeId param) {
    constexpr unsigned size = sizeof(KeyTypeId);
    constexpr unsigned bits = size * 8u;
    std::array<char, size> key_type{};

    boost::endian::endian_buffer<boost::endian::order::big, KeyTypeId, bits>
        buf(param);
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
