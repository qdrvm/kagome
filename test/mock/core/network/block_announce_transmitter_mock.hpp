/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/block_announce_transmitter.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class BlockAnnounceTransmitterMock : public BlockAnnounceTransmitter {
   public:
    MOCK_METHOD(void, blockAnnounce, (const network::BlockAnnounce &), ());
    void blockAnnounce(network::BlockAnnounce &&msg) override {
      blockAnnounce(msg);
    }
  };

}  // namespace kagome::network
