/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/block_announce_observer.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class BlockAnnounceObserverMock final : public BlockAnnounceObserver {
   public:
    MOCK_METHOD(void,
                onBlockAnnounceHandshake,
                (const libp2p::peer::PeerId &peer_id,
                 const network::BlockAnnounceHandshake &handshake),
                (override));

    MOCK_METHOD(void,
                onBlockAnnounce,
                (const libp2p::peer::PeerId &peer_id,
                 const network::BlockAnnounce &announce),
                (override));
  };

}  // namespace kagome::network
