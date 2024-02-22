/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "mock/core/network/protocol_base_mock.hpp"
#include "network/protocols/beefy_protocol.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class BeefyProtocolMock : public ProtocolBaseMock, public BeefyProtocol {
   public:
    MOCK_METHOD(void,
                broadcast,
                (std::shared_ptr<consensus::beefy::BeefyGossipMessage>),
                (override));
  };

}  // namespace kagome::network

// Trick to make mock-method compilable
namespace kagome::consensus::beefy {
  inline auto &operator<<(std::ostream &s, const VoteMessage &) {
    return s;
  }
  inline auto &operator<<(std::ostream &s, const SignedCommitment &) {
    return s;
  }
}  // namespace kagome::consensus::beefy
