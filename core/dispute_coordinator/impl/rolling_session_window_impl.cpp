/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/rolling_session_window_impl.hpp"

#include "blockchain/block_tree.hpp"

namespace kagome::dispute {
  RollingSessionWindowImpl::RollingSessionWindowImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ParachainHost> api,
      primitives::BlockHash block_hash)
      : api_(std::move(api)), block_tree_(std::move(block_tree)) {
    BOOST_ASSERT(api_ != nullptr);

    // At first, determine session window start using the chain state.
    auto session_index_res = api_->session_index_for_child(block_hash);
    const auto &session_index = session_index_res.value();

    // We want to get the session index for the child of the last finalized
    // block.
    auto last_finalized = block_tree_->getLastFinalized();
    auto earliest_non_finalized_block_session_res =
        api_->session_index_for_child(last_finalized.hash);
    const auto &earliest_non_finalized_block_session =
        earliest_non_finalized_block_session_res.value();

    // This will increase the session window to cover the full non-finalized
    // chain.
    auto on_chain_window_start = earliest_non_finalized_block_session;
    if (session_index > kWindowSize) {
      auto min = session_index - (kWindowSize - 1);
      if (min < on_chain_window_start) {
        on_chain_window_start = min;
      }
    }

    auto window_start = on_chain_window_start;
    std::vector<SessionInfo> stored_sessions;

    // Fetch session information from DB.
    auto stored_window_res = load();
    if (stored_window_res.has_error()) {
      // Can't load
    }
    auto &stored_window_opt = stored_window_res.value();
    // Get the DB stored sessions and recompute window start based on DB data.
    if (stored_window_opt.has_value()) {
      auto &stored_window = stored_window_opt.value();
      // Check if DB is ancient.
      if (earliest_non_finalized_block_session
          > stored_window.earliest_session
                + stored_window.session_info.size()) {
        // If ancient, we scrap it and fetch from chain state.
        stored_window.session_info.clear();
      }

      // The session window might extend beyond the last finalized block, but
      // that's fine as we'll prune it at next update.
      if (not stored_window.session_info.empty()) {
        // If there is at least one entry in db, we always take the DB as source
        // of truth.
        window_start = stored_window.earliest_session;
      }

      stored_sessions = std::move(stored_window.session_info);
    }

    // Compute the amount of sessions missing from the window that will be
    // fetched from chain state.
    auto sessions_missing_count = session_index;
    // .saturating_sub(window_start)
    sessions_missing_count = (sessions_missing_count > window_start)
                               ? (sessions_missing_count - window_start)
                               : 0;
    // .saturating_add(1)
    sessions_missing_count +=
        (sessions_missing_count
         < std::numeric_limits<decltype(session_index)>::max())
            ? sessions_missing_count + 1
            : sessions_missing_count;
    // .saturating_sub(stored_sessions.len() as u32);
    sessions_missing_count =
        (sessions_missing_count > stored_sessions.size())
            ? (sessions_missing_count - stored_sessions.size())
            : 0;

    // Extend from chain state.
    if (sessions_missing_count > 0) {
      auto sessions_res = extend_sessions_from_chain_state(stored_sessions,
                                                           &mut sender,
                                                           block_hash,
                                                           &mut window_start,
                                                           session_index, );

      if (sessions_res.has_error()) {
        // todo generate error 'SessionsUnavailable'
        // Err(kind) => Err(SessionsUnavailable {
        //	kind,
        //	info: Some(SessionsUnavailableInfo {
        //		window_start,
        //		window_end: session_index,
        //		block_hash,
        //	}),
        //}),
      }

      session_info_ = std::move(sessions_res.value());
    }

    earliest_session_ = window_start;

    // db_params: Some(db_params),
  }

  std::optional<std::reference_wrapper<SessionInfo>>
  RollingSessionWindowImpl::session_info(SessionIndex index) {
    if (index < earliest_session_) {
      return std::nullopt;
    } else {
      return session_info_.at(index - earliest_session_);
    }
  }

  SessionIndex RollingSessionWindowImpl::earliest_session() const {
    return earliest_session_;
  }

  SessionIndex RollingSessionWindowImpl::latest_session() const {
    return earliest_session_ + session_info.size() - 1;
  }

  bool RollingSessionWindowImpl::contains(SessionIndex session_index) const {
    return session_index >= earliest_session_
       and session_index <= latest_session();
  }

  outcome::result<std::optional<StoredWindow>>
  RollingSessionWindowImpl::load() {
    //
  }
  outcome::result<void> RollingSessionWindowImpl::save(
      StoredWindow stored_window) {
    //
  }

}  // namespace kagome::dispute
