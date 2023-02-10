/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLERROR
#define KAGOME_NETWORK_PROTOCOLERROR

#include "outcome/outcome.hpp"

namespace kagome::network {

  enum class ProtocolError {
    GONE = 1,
    PROTOCOL_NOT_IMPLEMENTED,
    CAN_NOT_CREATE_HANDSHAKE,
    NODE_NOT_SYNCHRONIZED_YET,
    GENESIS_NO_MATCH,
    HANDSHAKE_ERROR
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::network, ProtocolError);

#endif  // KAGOME_NETWORK_PROTOCOLERROR
