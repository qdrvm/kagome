/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ERROR_HPP
#define KAGOME_SCALE_ERROR_HPP

#include <outcome/outcome.hpp>
#include "scale/types.hpp"

namespace kagome::common::scale {
  /**
   * @brief EncodeError enum provides error codes for Encode methods
   */
  enum class EncodeError {        // 0 is reserved for success
    kCompactIntegerIsTooBig = 1,  ///< compact integer can't be more than 2**536
    kCompactIntegerIsNegative,    ///< cannot compact-encode negative integers
    kWrongCategory,               ///< wrong compact category
    kNoAlternative,               ///< wrong cast to alternative
  };

  /**
   * @brief DecoderError enum provides codes of errors for Decoder methods
   */
  enum class DecodeError {  // 0 is reserved for success
    kNotEnoughData = 1,     ///< not enough data to decode value
    kUnexpectedValue,       ///< unexpected value
    kTooManyItems,          ///< too many items, cannot address them in memory
    kWrongTypeIndex,        ///< wrong type index, cannot decode variant
  };
}  // namespace kagome::common::scale

OUTCOME_HPP_DECLARE_ERROR(kagome::common::scale, EncodeError)
OUTCOME_HPP_DECLARE_ERROR(kagome::common::scale, DecodeError)

#endif  // KAGOME_SCALE_ERROR_HPP
