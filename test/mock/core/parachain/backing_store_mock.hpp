/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/backing/store.hpp"

namespace kagome::parachain {

  class BackingStoreMock : public BackingStore {
   public:
    MOCK_METHOD(
        std::optional<ImportResult>,
        put,
        (const RelayHash &,
        (const std::unordered_map<ParachainId, std::vector<ValidatorIndex>> &),
        Statement ,
        bool ),
        (override));

    MOCK_METHOD(std::vector<BackedCandidate>,
                get,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(void,
                add,
                (const primitives::BlockHash &, BackedCandidate &&),
                (override));

    MOCK_METHOD(std::optional<std::reference_wrapper<const StatementInfo>>,
                getCadidateInfo,
                (const primitives::BlockHash &, const network::CandidateHash &),
                (const, override));

    MOCK_METHOD(void,
                onActivateLeaf,
                (const RelayHash &),
                (override));

    MOCK_METHOD(void,
                onDeactivateLeaf,
                (const RelayHash &),
                (override));
  };

}  // namespace kagome::parachain
