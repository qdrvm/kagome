/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_SERVER_MOCK_HPP
#define KAGOME_PEER_SERVER_MOCK_HPP

#include "network/peer_server.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  struct PeerServerMock : public PeerServer {
    MOCK_METHOD0(start, void());

    MOCK_METHOD1(onBlocksRequest,
                 void(std::function<
                      outcome::result<BlocksResponse>(const BlocksRequest &)>));

    MOCK_METHOD1(onBlockAnnounce,
                 void(std::function<void(const BlockAnnounce &)>));
  };
}  // namespace kagome::network

#endif  // KAGOME_PEER_SERVER_MOCK_HPP
