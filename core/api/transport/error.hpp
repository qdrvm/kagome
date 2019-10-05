/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
