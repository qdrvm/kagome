/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BACKING_STORE_MOCK_HPP
#define KAGOME_BACKING_STORE_MOCK_HPP

#include <gmock/gmock.h>

#include "parachain/backing/store.hpp"

namespace kagome::parachain {

  class BackingStoreMock : public BackingStore {
   public:
    MOCK_METHOD(
        std::optional<ImportResult>,
        put,
        ((std::unordered_map<ParachainId, std::vector<ValidatorIndex>> const &),
         Statement),
        (override));

    MOCK_METHOD(std::vector<BackedCandidate>,
                get,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(void, remove, (const primitives::BlockHash &), (override));

    MOCK_METHOD(void,
                add,
                (const primitives::BlockHash &, BackedCandidate &&),
                (override));

    MOCK_METHOD(std::optional<network::CommittedCandidateReceipt>,
                get_candidate,
                (network::CandidateHash const &),
                (const, override));

    MOCK_METHOD(std::optional<std::reference_wrapper<StatementInfo const>>,
                get_validity_votes,
                (network::CandidateHash const &),
                (const, override));
  };

}  // namespace kagome::parachain

#endif /* KAGOME_BACKING_STORE_MOCK_HPP */
