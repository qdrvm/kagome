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

#include "api/extrinsic/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, ExtrinsicApiError, e) {
  using kagome::api::ExtrinsicApiError;
  switch (e) {
    case ExtrinsicApiError::INVALID_STATE_TRANSACTION:
      return "transaction is in invalid state";
    case ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION:
      return "transaction is in unknown state";
    case ExtrinsicApiError::DECODE_FAILURE:
      return "failed to decode value";
  }
  return "unknown extrinsic submission error";
}
