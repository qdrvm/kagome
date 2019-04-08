/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, EncodeError, e) {
  using kagome::scale::EncodeError;
  switch (e) {
    case EncodeError::kCompactIntegerIsNegative:
      return "compact integers cannot be negative";
    case EncodeError::kCompactIntegerIsTooBig:
      return "compact integers cannot be negative";
    case EncodeError::kWrongCategory:
      return "wrong compact encoding category";
    default:
      break;
  }
  return "unknown EncodeError";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, DecodeError, e) {
  using kagome::scale::DecodeError;
  switch (e) {
    case DecodeError::kNotEnoughData:
      return "not enough data to decode";
    case DecodeError::kUnexpectedValue:
      return "unexpected value occured";
    case DecodeError::kTooManyItems:
      return "collection has too many items, unable to unpack";
    case DecodeError::kWrongTypeIndex:
      return "wrong type index, cannot decode variant";
    default:
      break;
  }
  return "unknown DecodeError";
}
