/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/rolling_session_window_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "dispute_coordinator/impl/errors.hpp"

namespace kagome::dispute {

  outcome::result<std::unique_ptr<RollingSessionWindow>>
  RollingSessionWindowImpl::create(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<ParachainHost> api,
      primitives::BlockHash block_hash,
      log::Logger log) {
    BOOST_ASSERT(block_tree != nullptr);
    BOOST_ASSERT(api != nullptr);
    BOOST_ASSERT(log != nullptr);

    // At first, determine session window start using the chain state.
    auto session_index_res = api->session_index_for_child(block_hash);
    if (session_index_res.has_error()) {
      SL_DEBUG(log,
               "Call 'session_index_for_child' was failed: {}",
               session_index_res.error());
      return SessionObtainingError::SessionsUnavailable;
    }
    const auto &session_index = session_index_res.value();

    // We want to get the session index for the child of the last finalized
    // block.
    auto last_finalized = block_tree->getLastFinalized();
    auto earliest_non_finalized_block_session_res =
        api->session_index_for_child(last_finalized.hash);
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
      return stored_window_res.as_failure();
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
        (sessions_missing_count < std::numeric_limits<SessionIndex>::max())
            ? sessions_missing_count + 1
            : sessions_missing_count;
    // .saturating_sub(stored_sessions.len() as u32);
    sessions_missing_count =
        (sessions_missing_count > stored_sessions.size())
            ? (sessions_missing_count - stored_sessions.size())
            : 0;

    // Extend from chain state.
    std::vector<SessionInfo> sessions;
    if (sessions_missing_count > 0) {
      auto sessions_res =
          extend_sessions_from_chain_state(api,
                                           std::move(stored_sessions),
                                           block_hash,
                                           window_start,
                                           session_index);

      if (sessions_res.has_error()) {
        return SessionObtainingError::SessionsUnavailable;
      }

      sessions = std::move(sessions_res.value());
    } else {
      sessions = std::move(stored_sessions);
    }

    RollingSessionWindowImpl rsw;
    rsw.log_ = std::move(log);
    rsw.api_ = api;
    rsw.block_tree_ = block_tree;
    rsw.earliest_session_ = window_start;
    rsw.session_info_ = sessions;
    rsw.window_size_ = kWindowSize;

    return std::make_unique<RollingSessionWindowImpl>(std::move(rsw));
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
    return earliest_session_ + session_info_.size() - 1;
  }

  bool RollingSessionWindowImpl::contains(SessionIndex session_index) const {
    return session_index >= earliest_session_
       and session_index <= latest_session();
  }

  outcome::result<std::optional<StoredWindow>>
  RollingSessionWindowImpl::load() {
    return boost::system::error_code{};  // FIXME
  }

  outcome::result<void> RollingSessionWindowImpl::save(
      StoredWindow stored_window) {
    return boost::system::error_code{};  // FIXME
  }

  outcome::result<SessionWindowUpdate>
  RollingSessionWindowImpl::cache_session_info_for_head(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(session_index, api_->session_index_for_child(block_hash));

    auto latest = latest_session();

    // Either cached or ancient.
    if (session_index <= latest) {
      return SessionWindowUnchanged{};
    }

    auto last_finalized = block_tree_->getLastFinalized();
    OUTCOME_TRY(earliest_non_finalized_block_session,
                api_->session_index_for_child(last_finalized.hash));

    auto old_window_start = earliest_session_;
    auto old_window_end = latest;

    // Ensure we keep sessions up to last finalized block by adjusting the
    // window start. This will increase the session window to cover the full
    // unfinalized chain.
    auto window_start = std::min<SessionIndex>(
        (session_index >= window_size_) ? (session_index - (window_size_ - 1))
                                        : 0,
        earliest_non_finalized_block_session);

    // Never look back past earliest session, since if sessions beyond were not
    // needed or available in the past remains valid for the future (window only
    // advances forward).
    window_start = std::max(window_start, earliest_session_);

    auto sessions = session_info_;

    auto sessions_out_of_window = (window_start >= old_window_start)
                                    ? (window_start - old_window_start)
                                    : 0;

    if (sessions_out_of_window < sessions.size()) {
      // Drop sessions based on how much the window advanced.
      sessions.erase(sessions.begin(),
                     sessions.begin() + sessions_out_of_window);
    } else {
      // Window has jumped such that we need to fetch all sessions from on
      // chain.
      sessions.clear();
    };

    auto res = extend_sessions_from_chain_state(
        api_, sessions, block_hash, window_start, session_index);

    if (res.has_error()) {
      return SessionObtainingError::SessionsUnavailable;
    } else {
      SessionWindowAdvanced update{
          .prev_window_start = old_window_start,
          .prev_window_end = old_window_end,
          .new_window_start = window_start,
          .new_window_end = session_index,
      };

      session_info_ = std::move(res.value());

      // we need to account for this case:
      // window_start ................................... session_index
      //              old_window_start ........... latest
      earliest_session_ = std::max(window_start, old_window_start);

      // Update current window in DB.
      save(StoredWindow{
          .earliest_session = earliest_session_,
          .session_info = session_info_,
      });

      return update;
    }
  }

  outcome::result<std::vector<SessionInfo>>
  RollingSessionWindowImpl::extend_sessions_from_chain_state(
      const std::shared_ptr<ParachainHost> &api,
      std::vector<SessionInfo> stored_sessions,
      const primitives::BlockHash &block_hash,
      SessionIndex &window_start,
      SessionIndex end_inclusive) {
    // Start from the db sessions.
    auto sessions = std::move(stored_sessions);

    // We allow session fetch failures only if we won't create a gap in the
    // window by doing so. If `allow_failure` is set to true here, fetching
    // errors are ignored until we get a first session.
    auto allow_failure = sessions.empty();

    auto start = window_start + sessions.size();

    for (auto i = start; i <= end_inclusive; ++i) {
      auto session_info_res = api->session_info(block_hash, i);

      if (session_info_res.has_value()) {
        auto &session_info_opt = session_info_res.value();
        if (session_info_opt.has_value()) {
          auto &session_info = session_info_opt.value();

          // We do not allow failure anymore after having at least 1 session in
          // window.
          allow_failure = false;
          sessions.push_back(std::move(session_info));

        } else if (not allow_failure) {
          return SessionObtainingError::Missing;

        } else {
          // Handle `allow_failure` true.
          // If we didn't get the session, we advance window start.
          ++window_start;
          // LOG-DEBUG: "Session info missing from runtime."
        }

      } else if (not allow_failure) {
        return SessionObtainingError::RuntimeApiError;

      } else {
        // Handle `allow_failure` true.
        // If we didn't get the session, we advance window start.
        ++window_start;
        // LOG-DEBUG: "Error while fetching session information."
      }
    }

    return sessions;
  }

}  // namespace kagome::dispute
