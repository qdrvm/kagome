/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, EncodeError, e) {
  using kagome::scale::EncodeError;
  switch (e) {
    case EncodeError::NEGATIVE_COMPACT_INTEGER:
      return "SCALE encode: compact integers cannot be negative";
    case EncodeError::COMPACT_INTEGER_TOO_BIG:
      return "SCALE encode: compact integer is too big";
    case EncodeError::DEREF_NULLPOINTER:
      return "SCALE encode: attempt to dereference a null ptr";
  }
  return "unknown SCALE EncodeError";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, DecodeError, e) {
  using kagome::scale::DecodeError;
  switch (e) {
    case DecodeError::NOT_ENOUGH_DATA:
      return "SCALE decode: not enough data to decode a value";
    case DecodeError::UNEXPECTED_VALUE:
      return "SCALE decode: found an unexpected value when decoding ";
    case DecodeError::TOO_MANY_ITEMS:
      return "SCALE decode: collection has too many items or memory is "
             "out or data is damaged, unable to unpack";
    case DecodeError::WRONG_TYPE_INDEX:
      return "SCALE decode: wrong type index, cannot decode variant";
  }
  return "unknown SCALE DecodeError";
}
