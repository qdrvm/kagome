/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_ROLLINGSESSIONWINDOWIMPL
#define KAGOME_DISPUTE_ROLLINGSESSIONWINDOWIMPL

#include "dispute_coordinator/rolling_session_window.hpp"

#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::dispute {

  using network::CandidateReceipt;
  using network::DisputeStatement;
  using network::SessionIndex;
  using network::ValidatorIndex;
  using network::Vote;
  using parachain::CandidateHash;
  using parachain::ValidatorSignature;
  using runtime::ParachainHost;
  using runtime::SessionInfo;

  class RollingSessionWindowImpl final : public RollingSessionWindow {
   public:
    static const size_t kWindowSize = 6;

    static outcome::result<std::unique_ptr<RollingSessionWindow>> create(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<ParachainHost> api,
        primitives::BlockHash block_hash);

    std::optional<std::reference_wrapper<SessionInfo>> session_info(
        SessionIndex index) override;

    SessionIndex earliest_session() const override;

    SessionIndex latest_session() const override;

    bool contains(SessionIndex session_index) const override;

    outcome::result<SessionWindowUpdate> cache_session_info_for_head(
        const primitives::BlockHash &block_hash) override;

   private:
    static outcome::result<std::optional<StoredWindow>> load();
    outcome::result<void> save(StoredWindow stored_window);

    outcome::result<SessionIndex> get_session_index_for_child(
        const primitives::BlockHash &block_hash);

    /// Attempts to extend db stored sessions with sessions missing between
    /// `start` and up to `end_inclusive`.
    /// Runtime session info fetching errors are ignored if that doesn't create
    /// a gap in the window.
    static outcome::result<std::vector<SessionInfo>>
    extend_sessions_from_chain_state(const std::shared_ptr<ParachainHost> &api,
                                     std::vector<SessionInfo> stored_sessions,
                                     const primitives::BlockHash &block_hash,
                                     SessionIndex &window_start,
                                     SessionIndex end_inclusive);

    std::shared_ptr<ParachainHost> api_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    SessionIndex earliest_session_;
    std::vector<SessionInfo> session_info_;
    SessionIndex window_size_;

    // The option is just to enable some approval-voting tests to force feed
    // sessions in the window without dealing with the DB.
    // Option<DatabaseParams> db_params;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_ROLLINGSESSIONWINDOWIMPL
