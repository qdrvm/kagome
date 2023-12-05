/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
