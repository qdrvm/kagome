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

    RollingSessionWindowImpl(std::shared_ptr<blockchain::BlockTree> block_tree,
                             std::shared_ptr<ParachainHost> api,
                             primitives::BlockHash block_hash);

    std::optional<std::reference_wrapper<SessionInfo>> session_info(
        SessionIndex index) override;

    SessionIndex earliest_session() const override;

    SessionIndex latest_session() const override;

    bool contains(SessionIndex session_index) const override;

   private:
    outcome::result<std::optional<StoredWindow>> load();
    outcome::result<void> save(StoredWindow stored_window);

    std::shared_ptr<ParachainHost> api_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    SessionIndex earliest_session_;
    std::vector<SessionInfo> session_info_;

    // The option is just to enable some approval-voting tests to force feed
    // sessions in the window without dealing with the DB.
    // Option<DatabaseParams> db_params;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_ROLLINGSESSIONWINDOWIMPL
