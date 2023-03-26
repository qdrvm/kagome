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
    using AvailableData = runtime::AvailableData;
    using CandidateHash = network::CandidateHash;
    using ValidatorIndex = network::ValidatorIndex;
    using ErasureChunk = network::ErasureChunk;
    using ParachainBlock = network::ParachainBlock;
    using PersistedValidationData = runtime::PersistedValidationData;

    virtual ~AvailabilityStore() = default;

    /// Has ErasureChunk
    virtual bool hasChunk(const CandidateHash &candidate_hash,
                          ValidatorIndex index) const = 0;
    /// Has PoV
    virtual bool hasPov(const CandidateHash &candidate_hash) const = 0;
    /// Has PersistedValidationData
    virtual bool hasData(const CandidateHash &candidate_hash) const = 0;
    /// Get ErasureChunk
    virtual std::optional<ErasureChunk> getChunk(
        const CandidateHash &candidate_hash, ValidatorIndex index) const = 0;
    /// Get PoV
    virtual std::optional<ParachainBlock> getPov(
        const CandidateHash &candidate_hash) const = 0;
    /// Get AvailableData (PoV and PersistedValidationData)
    virtual std::optional<AvailableData> getPovAndData(
        const CandidateHash &candidate_hash) const = 0;
    // Get list of ErasureChunk
    virtual std::vector<ErasureChunk> getChunks(
        const CandidateHash &candidate_hash) const = 0;
    /// Store ErasureChunk
    virtual void storeData(network::RelayHash const &relay_parent,
                           CandidateHash const &candidate_hash,
                           std::vector<ErasureChunk> &&chunks,
                           ParachainBlock const &pov,
                           PersistedValidationData const &data) = 0;
    /// Clears all data according to this relay_parent
    virtual void remove(network::RelayHash const &relay_parent) = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_HPP
