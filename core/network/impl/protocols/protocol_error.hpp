/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::network {

  enum class ProtocolError {
    GONE = 1,
    PROTOCOL_NOT_IMPLEMENTED,
    CAN_NOT_CREATE_HANDSHAKE,
    NODE_NOT_SYNCHRONIZED_YET,
    GENESIS_NO_MATCH,
    HANDSHAKE_ERROR,
    NO_RESPONSE,
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::network, ProtocolError);
