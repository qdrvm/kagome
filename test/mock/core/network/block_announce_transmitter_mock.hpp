/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK
#define KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK

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

#endif  // KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK
