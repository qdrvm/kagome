/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/storage_impl.hpp"

#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::dispute {

  StorageImpl::StorageImpl(std::shared_ptr<storage::SpacedStorage> storage)
      : storage_(std::move(storage)) {
    //
  }

  std::optional<SessionIndex> StorageImpl::load_earliest_session() {
    if (earliest_session_.has_value()) {
      return earliest_session_.value();
    }

    // load from base
    // see: {polkadot}/node/core/dispute-coordinator/src/backend.rs:101
    throw std::runtime_error("need to implement");
    return std::nullopt;
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
    if (it != candidate_votes_.end()) {
      return it->second;
    }
    // load from base
    // see: {polkadot}/node/core/dispute-coordinator/src/backend.rs:123
    throw std::runtime_error("need to implement");
    return std::nullopt;
  }

  void StorageImpl::write_recent_disputes(RecentDisputes recent_disputes) {
    recent_disputes_ = std::move(recent_disputes);

    // save to base
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/backend.rs#L136
    throw std::runtime_error("need to implement");
  }

}  // namespace kagome::dispute
