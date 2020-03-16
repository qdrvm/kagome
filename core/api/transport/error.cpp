/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/error.hpp"

namespace kagome::api
{
extern std::error_code make_error_code(ApiTransportError e)
{ return {static_cast<int>(e), __libp2p::Category<ApiTransportError>::get()}; }
};

template<>
std::string __libp2p::Category<kagome::api::ApiTransportError>::toString(kagome::api::ApiTransportError e) {
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
