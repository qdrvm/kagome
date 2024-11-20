/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/availability/store/store.hpp"

#include <unordered_map>
#include <unordered_set>
#include "application/app_state_manager.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/safe_object.hpp"

namespace kagome::parachain {
  class AvailabilityStoreImpl
      : public AvailabilityStore,
        public std::enable_shared_from_this<AvailabilityStoreImpl> {
   public:
    AvailabilityStoreImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        clock::SteadyClock &steady_clock,
        std::shared_ptr<storage::SpacedStorage> storage,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);
    ~AvailabilityStoreImpl() override = default;

    bool start();

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
    void storeData(const network::RelayHash &relay_parent,
                   const CandidateHash &candidate_hash,
                   std::vector<ErasureChunk> &&chunks,
                   const ParachainBlock &pov,
                   const PersistedValidationData &data) override;
    void putChunk(const network::RelayHash &relay_parent,
                  const CandidateHash &candidate_hash,
                  ErasureChunk &&chunk) override;
    void remove(const network::RelayHash &relay_parent) override;
    void printStoragesLoad() override;

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
      std::deque<std::pair<uint64_t, network::RelayHash>>
          candidates_living_keeper_;
    };

    void prune_candidates_no_lock(State &state);
    void remove_no_lock(State &state, const network::RelayHash &relay_parent);

    log::Logger logger = log::createLogger("AvailabilityStore", "parachain");
    clock::SteadyClock &steady_clock_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    primitives::events::ChainSub chain_sub_;
    SafeObject<State> state_{};
  };
}  // namespace kagome::parachain
