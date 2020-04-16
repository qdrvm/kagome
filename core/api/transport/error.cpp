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

#include "api/transport/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, ApiTransportError, e) {
  using kagome::api::ApiTransportError;
  switch (e) {
    case ApiTransportError::FAILED_SET_OPTION:
      return "cannot set an option";
    case ApiTransportError::FAILED_START_LISTENING:
      return "cannot start listening, invalid address or port is busy";
    case ApiTransportError::LISTENER_ALREADY_STARTED:
      return "cannot start listener, already started";
    case ApiTransportError::CANNOT_ACCEPT_LISTENER_NOT_WORKING:
      return " cannot accept new connection, state mismatch";
  }
  return "unknown extrinsic submission error";
}
