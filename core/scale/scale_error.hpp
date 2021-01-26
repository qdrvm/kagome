/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ERROR_HPP
#define KAGOME_SCALE_ERROR_HPP

#include <outcome/outcome.hpp>
#include "scale/types.hpp"

namespace kagome::scale {
  /**
   * @brief EncodeError enum provides error codes for Encode methods
   */
  enum class EncodeError {        // 0 is reserved for success
    COMPACT_INTEGER_TOO_BIG = 1,  ///< compact integer can't be more than 2**536
    NEGATIVE_COMPACT_INTEGER,     ///< cannot compact-encode negative integers
    DEREF_NULLPOINTER,            ///< dereferencing a null pointer
  };

  /**
   * @brief DecoderError enum provides codes of errors for Decoder methods
   */
  enum class DecodeError {  // 0 is reserved for success
    NOT_ENOUGH_DATA = 1,    ///< not enough data to decode a value
    UNEXPECTED_VALUE,       ///< unexpected value
    TOO_MANY_ITEMS,         ///< too many items, cannot address them in memory
    WRONG_TYPE_INDEX        ///< wrong type index, cannot decode variant
  };

}  // namespace kagome::scale

OUTCOME_HPP_DECLARE_ERROR(kagome::scale, EncodeError)
OUTCOME_HPP_DECLARE_ERROR(kagome::scale, DecodeError)

#endif  // KAGOME_SCALE_ERROR_HPP
