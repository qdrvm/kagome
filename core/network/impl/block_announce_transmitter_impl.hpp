/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL
#define KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL

#include "network/block_announce_transmitter.hpp"

namespace kagome::network {
  class Router;

  class BlockAnnounceTransmitterImpl final : public BlockAnnounceTransmitter {
   public:
    BlockAnnounceTransmitterImpl(std::shared_ptr<Router> router);

    void blockAnnounce(BlockAnnounce &&announce) override;

   private:
    std::shared_ptr<Router> router_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL
