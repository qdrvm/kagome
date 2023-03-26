/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP

#include "parachain/availability/store/store.hpp"

#include <unordered_map>
#include <unordered_set>
#include "utils/safe_object.hpp"

namespace kagome::parachain {
  class AvailabilityStoreImpl : public AvailabilityStore {
   public:
    ~AvailabilityStoreImpl() override = default;

    bool hasChunk(const CandidateHash &candidate_hash,
                  ValidatorIndex index) const override;
    bool hasPov(const CandidateHash &candidate_hash) const override;
    bool hasData(const CandidateHash &candidate_hash) const override;
    std::optional<ErasureChunk> getChunk(const CandidateHash &candidate_hash,
                                         ValidatorIndex index) const override;
    std::optional<ParachainBlock> getPov(
        const CandidateHash &candidate_hash) const override;
    std::optional<AvailableData> getPovAndData(
        const CandidateHash &candidate_hash) const override;
    std::vector<ErasureChunk> getChunks(
        const CandidateHash &candidate_hash) const override;
    void storeData(network::RelayHash const &relay_parent,
                   CandidateHash const &candidate_hash,
                   std::vector<ErasureChunk> &&chunks,
                   ParachainBlock const &pov,
                   PersistedValidationData const &data) override;
    void remove(network::RelayHash const &relay_parent) override;

   private:
    struct PerCandidate {
      std::unordered_map<ValidatorIndex, ErasureChunk> chunks{};
      std::optional<ParachainBlock> pov{};
      std::optional<PersistedValidationData> data{};
    };

    struct State {
      std::unordered_map<CandidateHash, PerCandidate> per_candidate_{};
      std::unordered_map<network::RelayHash, std::unordered_set<CandidateHash>>
          candidates_{};
    };

    SafeObject<State> state_{};
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_STORE_STORE_IMPL_HPP
