/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/protocol_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, ProtocolError, e) {
  using E = kagome::network::ProtocolError;
  switch (e) {
    case E::GONE:
      return "Protocol was switched off";
    case E::PROTOCOL_NOT_IMPLEMENTED:
      return "Protocol is not implemented";
    case E::CAN_NOT_CREATE_STATUS:
      return "Can not create status";
    case E::NODE_NOT_SYNCHRONIZED_YET:
      return "Node not synchrinized yet";
  }
  return "Unknown error (kagome::network::ProtocolError)";
}
