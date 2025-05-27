/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/i_validator_side.hpp"

#include <gmock/gmock.h>

namespace kagome::parachain {

  class ValidatorSideMock : public ValidatorSide {
   public:
    MOCK_METHOD(
        void,
        updateActiveLeaves,
        ((const std::unordered_map<Hash, ActiveLeafState> &active_leaves),
         const ImplicitView &implicit_view),
        (override));
    MOCK_METHOD(bool,
                canProcessAdvertisement,
                (const RelayHash &relay_parent,
                 const ParachainId &para_id,
                 const runtime::ClaimQueueSnapshot &claim_queue),
                (override));
    MOCK_METHOD(void,
                registerCollationFetch,
                (const RelayHash &relay_parent, const ParachainId &para_id),
                (override));
    MOCK_METHOD(void,
                completeCollationFetch,
                (const RelayHash &relay_parent, const ParachainId &para_id),
                (override));
    MOCK_METHOD(
        (std::optional<std::pair<kagome::crypto::Sr25519PublicKey,
                                 std::optional<CandidateHash>>>),
        getNextCollationToFetch,
        (const RelayHash &relay_parent,
         (const std::pair<kagome::crypto::Sr25519PublicKey,
                          std::optional<CandidateHash>> &previous_fetch)),
        (override));
    MOCK_METHOD(void,
                addFetchedCandidate,
                (const network::FetchedCollation &collation,
                 const network::CollationEvent &event),
                (override));
    MOCK_METHOD(void,
                removeFetchedCandidate,
                (const network::FetchedCollation &collation),
                (override));
    MOCK_METHOD(void,
                blockFromSeconding,
                (const BlockedCollationId &id,
                 network::PendingCollationFetch &&fetch),
                (override));
    MOCK_METHOD((std::vector<network::PendingCollationFetch>),
                takeBlockedCollations,
                (const BlockedCollationId &id),
                (override));
    MOCK_METHOD((std::unordered_map<Hash, ActiveLeafState> &),
                activeLeaves,
                (),
                (override));

    MOCK_METHOD(bool,
                hasBlockedCollations,
                (const BlockedCollationId &id),
                (const, override));
    MOCK_METHOD((const FetchedCandidatesMap &),
                fetchedCandidates,
                (),
                (const, override));
    MOCK_METHOD((FetchedCandidatesMap &), fetchedCandidates, (), (override));
  };

}  // namespace kagome::parachain
