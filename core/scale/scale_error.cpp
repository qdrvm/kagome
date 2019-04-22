/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, EncodeError, e) {
  using kagome::scale::EncodeError;
  switch (e) {
    case EncodeError::NEGATIVE_COMPACT_INTEGER:
      return "compact integers cannot be negative";
    case EncodeError::COMPACT_INTEGER_TOO_BIG:
      return "compact integers cannot be negative";
    case EncodeError::WRONG_CATEGORY:
      return "wrong compact encoding category";
    default:
      break;
  }
  return "unknown EncodeError";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, DecodeError, e) {
  using kagome::scale::DecodeError;
  switch (e) {
    case DecodeError::NOT_ENOUGH_DATA:
      return "not enough data to decode";
    case DecodeError::UNEXPECTED_VALUE:
      return "unexpected value occured";
    case DecodeError::TOO_MANY_ITEMS:
      return "collection has too many items, unable to unpack";
    case DecodeError::WRONG_TYPE_INDEX:
      return "wrong type index, cannot decode variant";
    default:
      break;
  }
  return "unknown DecodeError";
}
