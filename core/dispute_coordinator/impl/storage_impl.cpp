/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/storage_impl.hpp"

namespace kagome::dispute {

  StorageImpl::StorageImpl() {
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

  std::optional<RecentDisputes> StorageImpl::load_recent_disputes() {
    if (recent_disputes_.has_value()) {
      return recent_disputes_.value();
    }
    // load from base
    // see: {polkadot}/node/core/dispute-coordinator/src/backend.rs:110
    throw std::runtime_error("need to implement");
    return std::nullopt;
  }

  std::optional<CandidateVotes> StorageImpl::load_candidate_votes(
      SessionIndex session, const CandidateHash &candidate_hash) {
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
