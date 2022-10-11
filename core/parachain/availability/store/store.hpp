/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_HPP

#include <optional>

#include "network/types/collator_messages.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  /// Stores ErasureChunk, PoV and PersistedValidationData
  class AvailabilityStore {
   public:
    using CandidateHash = network::CandidateHash;
    using ValidatorIndex = network::ValidatorIndex;
    using ErasureChunk = network::ErasureChunk;
    using ParachainBlock = network::ParachainBlock;
    using PersistedValidationData = runtime::PersistedValidationData;

    virtual ~AvailabilityStore() = default;

    /// Has ErasureChunk
    virtual bool hasChunk(const CandidateHash &candidate_hash,
                          ValidatorIndex index) = 0;
    /// Has PoV
    virtual bool hasPov(const CandidateHash &candidate_hash) = 0;
    /// Has PersistedValidationData
    virtual bool hasData(const CandidateHash &candidate_hash) = 0;
    /// Get ErasureChunk
    virtual std::optional<ErasureChunk> getChunk(
        const CandidateHash &candidate_hash, ValidatorIndex index) = 0;
    /// Get PoV
    virtual std::optional<ParachainBlock> getPov(
        const CandidateHash &candidate_hash) = 0;
    /// Store ErasureChunk
    virtual void putChunk(const CandidateHash &candidate_hash,
                          const ErasureChunk &chunk) = 0;
    /// Store PoV
    virtual void putPov(const CandidateHash &candidate_hash,
                        const ParachainBlock &pov) = 0;
    /// Store PersistedValidationData
    virtual void putData(const CandidateHash &candidate_hash,
                         const PersistedValidationData &data) = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_HPP
