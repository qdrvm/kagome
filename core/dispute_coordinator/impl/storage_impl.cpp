/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/storage_impl.hpp"

#include "scale/kagome_scale.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::dispute {

  StorageImpl::StorageImpl(std::shared_ptr<storage::SpacedStorage> storage)
      : storage_(std::move(storage)) {
    //
  }

  outcome::result<std::optional<RecentDisputes>>
  StorageImpl::load_recent_disputes() {
    if (not recent_disputes_.has_value()) {
      // load from base
      auto dispute_space = storage_->getSpace(storage::Space::kDisputeData);
      OUTCOME_TRY(data_opt,
                  dispute_space->tryGet(storage::kRecentDisputeLookupKey));
      if (not data_opt.has_value()) {
        return std::nullopt;
      }
      OUTCOME_TRY(recent_disputes,
                  scale::decode<RecentDisputes>(data_opt.value()));

      recent_disputes_ = std::move(recent_disputes);
    }

    return recent_disputes_.value();
  }

  outcome::result<std::optional<CandidateVotes>>
  StorageImpl::load_candidate_votes(SessionIndex session,
                                    const CandidateHash &candidate_hash) {
    auto it = candidate_votes_.find(std::tie(session, candidate_hash));
    if (it == candidate_votes_.end()) {
      auto dispute_space = storage_->getSpace(storage::Space::kDisputeData);
      OUTCOME_TRY(encoded_opt,
                  dispute_space->tryGet(storage::kCandidateVotesLookupKey(
                      session, candidate_hash)));
      if (not encoded_opt.has_value()) {
        it = candidate_votes_
                 .emplace(std::tie(session, candidate_hash), std::nullopt)
                 .first;
      } else {
        auto &encoded = encoded_opt.value();
        OUTCOME_TRY(candidate_votes, scale::decode<CandidateVotes>(encoded));
        it = candidate_votes_
                 .emplace(std::tie(session, candidate_hash),
                          std::move(candidate_votes))
                 .first;
      }
    }

    return it->second;
  }

  void StorageImpl::write_candidate_votes(SessionIndex session,
                                          const CandidateHash &candidate_hash,
                                          const CandidateVotes &votes) {
    candidate_votes_[std::tie(session, candidate_hash)] = votes;
    // TODO(xDimon): save to DB
  }

  void StorageImpl::write_recent_disputes(RecentDisputes recent_disputes) {
    recent_disputes_ = std::move(recent_disputes);

    // TODO(xDimon): save to DB
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/backend.rs#L136
  }

}  // namespace kagome::dispute
