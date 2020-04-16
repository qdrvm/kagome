/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    case EncodeError::WRONG_ALTERNATIVE:
      return "wrong cast to alternative";
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
    case DecodeError::INVALID_DATA:
      return "incorrect source data";
    case DecodeError::OUT_OF_BOUNDARIES:
      return "advance went out of boundaries";
  }
  return "unknown DecodeError";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::scale, CommonError, e) {
  using kagome::scale::CommonError;
  switch (e) {
    case CommonError::UNKNOWN_ERROR:
      return "unknown error";
  }
  return "unknown CommonError";
}
