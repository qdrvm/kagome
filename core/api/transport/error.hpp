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

#ifndef KAGOME_CORE_API_TRANSPORT_ERROR_HPP
#define KAGOME_CORE_API_TRANSPORT_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::api {
  enum class ApiTransportError {
    FAILED_SET_OPTION = 1,   // cannot set an option
    FAILED_START_LISTENING,  // cannot start listening, invalid address or port
                             // is busy
    LISTENER_ALREADY_STARTED,  // cannot start listener, already started
    CANNOT_ACCEPT_LISTENER_NOT_WORKING,  // cannot accept new connection, state
                                         // mismatch
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::api, ApiTransportError)

#endif  // KAGOME_CORE_API_TRANSPORT_ERROR_HPP
