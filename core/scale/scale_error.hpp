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
    WRONG_CATEGORY,               ///< wrong compact category
    WRONG_ALTERNATIVE,            ///< wrong cast to alternative
  };

  /**
   * @brief DecoderError enum provides codes of errors for Decoder methods
   */
  enum class DecodeError {  // 0 is reserved for success
    NOT_ENOUGH_DATA = 1,    ///< not enough data to decode value
    UNEXPECTED_VALUE,       ///< unexpected value
    TOO_MANY_ITEMS,         ///< too many items, cannot address them in memory
    WRONG_TYPE_INDEX,       ///< wrong type index, cannot decode variant
    INVALID_DATA,           ///< invalid data
    OUT_OF_BOUNDARIES       ///< advance went out of boundaries
  };

  /**
   * @brief common errors
   */
  enum class CommonError {
    UNKNOWN_ERROR = 1  ///< unknown error
  };
}  // namespace kagome::scale

OUTCOME_HPP_DECLARE_ERROR(kagome::scale, EncodeError)
OUTCOME_HPP_DECLARE_ERROR(kagome::scale, DecodeError)
OUTCOME_HPP_DECLARE_ERROR(kagome::scale, CommonError)

#endif  // KAGOME_SCALE_ERROR_HPP
