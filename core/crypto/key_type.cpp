/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_type.hpp"

#include <unordered_set>

#include <boost/endian/arithmetic.hpp>

namespace kagome::crypto {

  bool isSupportedKeyType(KeyTypeId k) {
    static const std::unordered_set<KeyTypeId> supported_types = {
        supported_key_types::kBabe,
        supported_key_types::kGran,
        supported_key_types::kAcco,
        supported_key_types::kImon,
        supported_key_types::kAudi,
        supported_key_types::kLp2p};

    return supported_types.count(k) > 0;
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
