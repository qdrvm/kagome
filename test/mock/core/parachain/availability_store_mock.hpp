/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/store/store.hpp"

#include <gmock/gmock.h>

namespace kagome::parachain {

  class AvailabilityStoreMock : public AvailabilityStore {
   public:
    MOCK_METHOD(bool,
                hasChunk,
                (const CandidateHash &candidate_hash, ChunkIndex index),
                (const, override));

    MOCK_METHOD(bool,
                hasPov,
                (const CandidateHash &candidate_hash),
                (const, override));

    MOCK_METHOD(bool,
                hasData,
                (const CandidateHash &candidate_hash),
                (const, override));

    MOCK_METHOD(std::optional<ErasureChunk>,
                getChunk,
                (const CandidateHash &candidate_hash, ChunkIndex index),
                (const, override));

    MOCK_METHOD(std::optional<ParachainBlock>,
                getPov,
                (const CandidateHash &candidate_hash),
                (const, override));

    MOCK_METHOD(std::optional<AvailableData>,
                getPovAndData,
                (const CandidateHash &candidate_hash),
                (const, override));

    MOCK_METHOD(std::vector<ErasureChunk>,
                getChunks,
                (const CandidateHash &candidate_hash),
                (const, override));

    MOCK_METHOD(void,
                storeData,
                (const network::RelayHash &,
                 const CandidateHash &,
                 std::vector<ErasureChunk> &&,
                 const ParachainBlock &,
                 const PersistedValidationData &),
                (override));

    MOCK_METHOD(void,
                putChunk,
                (const network::RelayHash &,
                 const CandidateHash &,
                 ErasureChunk &&),
                (override));

    MOCK_METHOD(void,
                remove,
                (const network::RelayHash &relay_parent),
                (override));

    MOCK_METHOD(void, printStoragesLoad, (), (override));
  };

}  // namespace kagome::parachain
