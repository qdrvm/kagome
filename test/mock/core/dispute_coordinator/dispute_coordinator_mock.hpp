/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORMOCK
#define KAGOME_DISPUTE_DISPUTECOORDINATORMOCK

#include "dispute_coordinator/dispute_coordinator.hpp"

#include <gmock/gmock.h>

namespace kagome::dispute {
  class DisputeCoordinatorMock : public DisputeCoordinator {
   public:
    MOCK_METHOD(void,
                getRecentDisputes,
                (CbOutcome<OutputDisputes> &&),
                (override));

    MOCK_METHOD(void,
                onParticipation,
                (const ParticipationStatement &),
                (override));

    MOCK_METHOD(void,
                getActiveDisputes,
                (CbOutcome<OutputDisputes> &&),
                (override));

    MOCK_METHOD(void,
                queryCandidateVotes,
                (const QueryCandidateVotes &,
                 CbOutcome<OutputCandidateVotes> &&),
                (override));

    MOCK_METHOD(void,
                issueLocalStatement,
                (SessionIndex, CandidateHash, CandidateReceipt, bool valid),
                (override));

    MOCK_METHOD(void,
                determineUndisputedChain,
                (primitives::BlockInfo,
                 std::vector<BlockDescription>,
                 CbOutcome<primitives::BlockInfo> &&),
                (override));

    MOCK_METHOD(void,
                getDisputeForInherentData,
                (const primitives::BlockInfo &,
                 std::function<void(MultiDisputeStatementSet)> &&),
                (override));
  };
}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORMOCK
