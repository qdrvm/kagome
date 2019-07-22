/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAD_ROUTING_TABLE_MOCK_HPP
#define KAGOME_KAD_ROUTING_TABLE_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/protocol/kademlia/routing_table.hpp"

namespace libp2p::protocol::kademlia {

  struct RoutingTableMock : public RoutingTable {
    ~RoutingTableMock() override = default;

    MOCK_METHOD1(update, outcome::result<peer::PeerId>(const peer::PeerId &));
    MOCK_METHOD1(remove, void(const NodeId &id));
    MOCK_CONST_METHOD0(getAllPeers, PeerIdVec());
    MOCK_METHOD3(getNearestPeers,
                 void(const NodeId &id, size_t count, PeerIdVecResultFunc f));
    MOCK_CONST_METHOD0(size, size_t());
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KAD_ROUTING_TABLE_MOCK_HPP
