/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP

#include "parachain/availability/store/store.hpp"

#include <unordered_map>
#include <unordered_set>

namespace kagome::parachain {
  class AvailabilityStoreImpl : public AvailabilityStore {
   public:
    ~AvailabilityStoreImpl() override = default;

    bool hasChunk(const CandidateHash &candidate_hash,
                  ValidatorIndex index) override;
    bool hasPov(const CandidateHash &candidate_hash) override;
    bool hasData(const CandidateHash &candidate_hash) override;
    std::optional<ErasureChunk> getChunk(const CandidateHash &candidate_hash,
                                         ValidatorIndex index) override;
    std::optional<ParachainBlock> getPov(
        const CandidateHash &candidate_hash) override;
    std::optional<AvailableData> getPovAndData(
        const CandidateHash &candidate_hash) override;
    std::vector<ErasureChunk> getChunks(
        const CandidateHash &candidate_hash) override;
    void putChunk(const CandidateHash &candidate_hash,
                  const ErasureChunk &chunk) override;
    void putChunkSet(const CandidateHash &candidate_hash,
                     std::vector<ErasureChunk> &&chunks) override;
    void putPov(const CandidateHash &candidate_hash,
                const ParachainBlock &pov) override;
    void putData(const CandidateHash &candidate_hash,
                 const PersistedValidationData &data) override;
    void registerCandidate(network::RelayHash const &relay_parent,
                           CandidateHash const &candidate_hash) override;
    void remove(network::RelayHash const &relay_parent) override;

   private:
    struct PerCandidate {
      std::unordered_map<ValidatorIndex, ErasureChunk> chunks;
      std::optional<ParachainBlock> pov;
      std::optional<PersistedValidationData> data;
    };

    std::unordered_map<CandidateHash, PerCandidate> per_candidate_;
    std::unordered_map<network::RelayHash, std::unordered_set<CandidateHash>>
        candidates_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP
