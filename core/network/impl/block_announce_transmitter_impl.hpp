/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL
#define KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL

#include "network/block_announce_transmitter.hpp"

#include "network/router.hpp"

namespace kagome::network {

  class BlockAnnounceTransmitterImpl final : public BlockAnnounceTransmitter {
   public:
    BlockAnnounceTransmitterImpl(std::shared_ptr<network::Router> router);

    void blockAnnounce(network::BlockAnnounce &&announce) override;

   private:
    std::shared_ptr<network::Router> router_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BLOCKANNOUNCETRANSMITTERIMPL
