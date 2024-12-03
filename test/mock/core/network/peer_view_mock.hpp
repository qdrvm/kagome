/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/peer_view.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  class PeerViewMock : public IPeerView {
   public:
    MOCK_METHOD(size_t,
                peersCount,
                (),
                (const, override));

    MOCK_METHOD(MyViewSubscriptionEnginePtr,
                getMyViewObservable,
                (),
                (override));

    MOCK_METHOD(PeerViewSubscriptionEnginePtr,
                getRemoteViewObservable,
                (),
                (override));

    MOCK_METHOD(void,
                removePeer,
                (const PeerId &),
                (override));

    MOCK_METHOD(void,
                updateRemoteView,
                (const PeerId &, network::View &&),
                (override));

    MOCK_METHOD(const View &,
                getMyView,
                (),
                (const, override));
  };
}  // namespace kagome::network
