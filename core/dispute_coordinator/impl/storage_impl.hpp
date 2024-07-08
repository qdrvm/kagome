/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/storage.hpp"

#include "utils/tuple_hash.hpp"

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::dispute {

  class StorageImpl final : public Storage {
   public:
    StorageImpl(std::shared_ptr<storage::SpacedStorage> storage);

    outcome::result<std::optional<RecentDisputes>> load_recent_disputes()
        override;

    outcome::result<std::optional<CandidateVotes>> load_candidate_votes(
        SessionIndex session, const CandidateHash &candidate_hash) override;

    void write_candidate_votes(SessionIndex session,
                               const CandidateHash &candidate_hash,
                               const CandidateVotes &votes) override;

    void write_recent_disputes(RecentDisputes recent_disputes) override;

   private:
    std::shared_ptr<storage::SpacedStorage> storage_;

    // `nullopt` means unchanged.
    std::optional<RecentDisputes> recent_disputes_{};
    // `nullopt` means deleted, missing means query inner.
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>,
                       std::optional<CandidateVotes>>
        candidate_votes_{};
  };
}  // namespace kagome::dispute
