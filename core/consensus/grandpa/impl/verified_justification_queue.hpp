/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <set>

#include "consensus/grandpa/i_verified_justification_queue.hpp"
#include "consensus/grandpa/structs.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "utils/weak_io_context.hpp"

namespace kagome {
  class ThreadHandler;
}

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class MainThreadPool;
}

namespace kagome::consensus {
  class Timeline;
}

namespace kagome::network {
  class Synchronizer;
}

namespace kagome::consensus::grandpa {
  class AuthorityManager;

  class VerifiedJustificationQueue
      : public IVerifiedJustificationQueue,
        public std::enable_shared_from_this<VerifiedJustificationQueue> {
   public:
    VerifiedJustificationQueue(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<common::MainThreadPool> main_thread_pool,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<AuthorityManager> authority_manager,
        LazySPtr<network::Synchronizer> synchronizer,
        LazySPtr<Timeline> timeline,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);

    bool start();
    void stop();

    void addVerified(AuthoritySetId set,
                     GrandpaJustification justification) override;

    void warp() override;

   private:
    void finalize(
        std::optional<AuthoritySetId> set,
        const consensus::grandpa::GrandpaJustification &justification);
    void verifiedLoop();
    void requiredLoop();
    void possibleLoop();
    void rangeLoop();

    std::shared_ptr<ThreadHandler> main_thread_handler_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<AuthorityManager> authority_manager_;
    LazySPtr<network::Synchronizer> synchronizer_;
    LazySPtr<Timeline> timeline_;
    primitives::events::ChainSub chain_sub_;
    log::Logger log_;

    using SetAndJustification = std::pair<AuthoritySetId, GrandpaJustification>;
    AuthoritySetId expected_;
    std::map<AuthoritySetId, SetAndJustification> verified_;
    std::optional<SetAndJustification> last_;
    std::set<primitives::BlockInfo> required_;
    std::vector<primitives::BlockInfo> possible_;
    primitives::BlockNumber range_ = 0;
    bool fetching_ = false;
  };
}  // namespace kagome::consensus::grandpa
