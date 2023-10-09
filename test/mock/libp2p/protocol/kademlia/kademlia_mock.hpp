/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_MOCK_HPP
#define LIBP2P_KADEMLIA_MOCK_HPP

#include <libp2p/protocol/kademlia/kademlia.hpp>

#include <gmock/gmock.h>

namespace libp2p::protocol::kademlia {
  struct KademliaMock : Kademlia {
    MOCK_METHOD(outcome::result<void>, putValue, (Key, Value), (override));

    MOCK_METHOD(outcome::result<void>,
                getValue,
                (const Key &, FoundValueHandler),
                (override));

    MOCK_METHOD(outcome::result<void>,
                provide,
                (const Key &, bool),
                (override));

    MOCK_METHOD(outcome::result<void>,
                findProviders,
                (const Key &, size_t, FoundProvidersHandler),
                (override));

    MOCK_METHOD(void, addPeer, (const PeerInfo &, bool, bool), (override));

    MOCK_METHOD(outcome::result<void>,
                findPeer,
                (const PeerId &, FoundPeerInfoHandler),
                (override));

    MOCK_METHOD(outcome::result<void>, bootstrap, (), (override));

    MOCK_METHOD(void, start, (), (override));
  };
}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_MOCK_HPP
