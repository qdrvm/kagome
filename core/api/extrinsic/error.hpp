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

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::api {
  enum class ExtrinsicApiError {
    INVALID_STATE_TRANSACTION = 1,  // transaction is in invalid state
    UNKNOWN_STATE_TRANSACTION,      // transaction is in unknown state
    DECODE_FAILURE                  // failed to decode value
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::api, ExtrinsicApiError)

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP
