/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPE_ENCODER_HPP
#define KAGOME_SCALE_TYPE_ENCODER_HPP

#include "common/scale/boolean.hpp"
#include "common/scale/compact.hpp"
#include "common/scale/fixedwidth.hpp"
#include "common/scale/types.hpp"
#include "common/scale/util.hpp"

namespace kagome::common::scale {
  template <class T>
  struct TypeEncoder {
    // if required to calculate size, we can add
    // canCalculateSize() and calculateSize(const T& item) methods
    bool encode(const T &item, Buffer &out) {
      static_assert(std::is_integral<T>(),
                    "Only integral types are supported. You need to define "
                    "your own TypeDecoder specialization for custom type.");
      impl::encodeInteger<T>(item, out);
      return true;
    };
  };

  template <>
  struct TypeEncoder<bool> {
    bool encode(bool item, Buffer &out) const {
      boolean::encodeBool(item, out);
      return true;
    }
  };

  template <>
  struct TypeEncoder<tribool> {
    bool encode(tribool item, Buffer &out) const {
      boolean::encodeTribool(item, out);
      return true;
    }
  };
}  // namespace kagome::common::scale

#endif  // KAGOME_SCALE_TYPE_ENCODER_HPP
