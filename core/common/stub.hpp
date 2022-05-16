/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STUB
#define KAGOME_STUB

#include "common/empty.hpp"
#include "common/tagged.hpp"

#include <typeinfo>

namespace kagome {
  namespace stub {}

  /// Stub-class for unimplemented types
  template <typename Tag = struct Unknown>
  struct Stub final : private Tagged<Empty, Tag> {
    bool operator==(const Stub &) const {
      return false;
    }
  };

  template <typename Tag>
  [[noreturn]] scale::ScaleEncoderStream &operator<<(
      scale::ScaleEncoderStream &s, const Stub<Tag> &data) {
    throw std::runtime_error(fmt::format(
        "Can not encode: encoding object is stabbed type tagged by {}",
        typeid(Tag).name()));
  }

  template <typename Tag>
  [[noreturn]] scale::ScaleDecoderStream &operator>>(
      scale::ScaleDecoderStream &s, Stub<Tag> &data) {
    throw std::runtime_error(fmt::format(
        "Can not decode: decoding object is stabbed type tagged by {}",
        typeid(Tag).name()));
  }

}  // namespace kagome

#endif  // KAGOME_STUB
