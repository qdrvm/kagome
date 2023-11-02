/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
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
      return "cannot accept new connection, state mismatch";
  }
  return "unknown transport error";
}
