/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPE_ENCODER_HPP
#define KAGOME_SCALE_TYPE_ENCODER_HPP

#include "scale/boolean.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_error.hpp"
#include "scale/types.hpp"
#include "scale/util.hpp"

// TODO(yuraz): PRE-119 conception of TypeEncoder/TypeDecoder needs to be
// refactored

namespace kagome::scale {
  template <class T>
  struct TypeEncoder {
    // if required to calculate size, we can add
    // canCalculateSize() and calculateSize(const T& item) methods
    outcome::result<void> encode(const T &item, common::Buffer &out) {
      static_assert(std::is_integral<T>(),
                    "Only integral types are supported. You need to define "
                    "your own TypeDecoder specialization for custom type.");
      impl::encodeInteger<T>(item, out);
      return outcome::success();
    };
  };

  /// encoder for bool
  template <>
  struct TypeEncoder<bool> {
    outcome::result<void> encode(bool item, common::Buffer &out) const {
      boolean::encodeBool(item, out);
      return outcome::success();
    }
  };

  /// encoder for tribool
  template <>
  struct TypeEncoder<tribool> {
    outcome::result<void> encode(tribool item, common::Buffer &out) const {
      boolean::encodeTribool(item, out);
      return outcome::success();
    }
  };

  /// encoder for std::pair
//  template <class F, class S>
//  struct TypeEncoder<std::pair<F, S>> {
//    outcome::result<void> encode(const std::pair<F, S> &pair,
//                                 common::Buffer &out) {
//      OUTCOME_TRY(TypeEncoder<F>{}.encode(pair.first, out));
//      OUTCOME_TRY(TypeEncoder<S>{}.encode(pair.second, out));
//      return outcome::success();
//    }
//  };
}  // namespace kagome::scale

#endif  // KAGOME_SCALE_TYPE_ENCODER_HPP
