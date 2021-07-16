/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK
#define KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK

#include "network/block_announce_transmitter.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class BlockAnnounceTransmitterMock : public BlockAnnounceTransmitter {
   public:
    void blockAnnounce(network::BlockAnnounce &&msg) {
      blockAnnounce_rv(msg);
    }
    MOCK_METHOD1(blockAnnounce_rv, void(const network::BlockAnnounce &));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERMOCK
