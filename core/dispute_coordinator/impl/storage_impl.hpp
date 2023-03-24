/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_STORAGEIMPL
#define KAGOME_DISPUTE_STORAGEIMPL

#include "dispute_coordinator/storage.hpp"

namespace kagome::dispute {

  class StorageImpl final : public Storage {
   public:
    StorageImpl();

    std::optional<SessionIndex> load_earliest_session() override;

    std::optional<RecentDisputes> load_recent_disputes() override;

    std::optional<CandidateVotes> load_candidate_votes(
        SessionIndex session, const CandidateHash &candidate_hash) override;

   private:
    // `nullopt` means unchanged.
    std::optional<SessionIndex> earliest_session_{};
    // `nullopt` means unchanged.
    std::optional<RecentDisputes> recent_disputes_{};
    // `nullopt` means deleted, missing means query inner.
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>,
                       std::optional<CandidateVotes>>
        candidate_votes_{};
  };
}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_STORAGEIMPL
