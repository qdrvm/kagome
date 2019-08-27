/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_CLIENT_MOCK_HPP
#define KAGOME_PEER_CLIENT_MOCK_HPP

#include "network/peer_client.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  struct PeerClientMock : public PeerClient {
    MOCK_CONST_METHOD2(
        blocksRequest,
        void(BlocksRequest,
             std::function<void(const outcome::result<BlocksResponse> &)>));

    MOCK_CONST_METHOD2(
        blockAnnounce,
        void(BlockAnnounce,
             std::function<void(const outcome::result<void> &)>));
  };
}  // namespace kagome::network

#endif  // KAGOME_PEER_CLIENT_MOCK_HPP
