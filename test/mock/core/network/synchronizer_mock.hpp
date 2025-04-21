/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/synchronizer.hpp"

#include "consensus/grandpa/structs.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class SynchronizerMock : public Synchronizer {
   public:
    MOCK_METHOD(void,
                addPeerKnownBlockInfo,
                (const BlockInfo &, const PeerId &),
                (override));

    MOCK_METHOD(void,
                onBlockAnnounce,
                (const BlockHeader &, const PeerId &),
                (override));

    MOCK_METHOD(bool,
                fetchJustification,
                (const primitives::BlockInfo &, CbResultVoid),
                (override));

    MOCK_METHOD(bool,
                fetchJustificationRange,
                (primitives::BlockNumber, FetchJustificationRangeCb),
                (override));

    MOCK_METHOD(void,
                syncState,
                (const primitives::BlockInfo &, SyncStateCb),
                ());

    MOCK_METHOD(bool,
                fetchHeadersBack,
                (const primitives::BlockInfo &,
                 primitives::BlockNumber,
                 bool,
                 CbResultVoid),
                (override));

    MOCK_METHOD(void,
                trySyncShortFork,
                (const PeerId &, const primitives::BlockInfo &),
                (override));

    MOCK_METHOD(void, unsafe, (PeerId, BlockNumber, UnsafeCb), (override));
  };

}  // namespace kagome::network
