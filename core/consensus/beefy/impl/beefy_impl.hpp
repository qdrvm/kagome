/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include <libp2p/basic/scheduler.hpp>

#include "consensus/beefy/beefy.hpp"
#include "consensus/beefy/types.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "primitives/justification.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome {
  class PoolHandler;
}

namespace kagome::application {
  class AppStateManager;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class MainPoolHandler;
  class WorkerPoolHandler;
}  // namespace kagome::common

namespace kagome::consensus {
  class Timeline;
}

namespace kagome::crypto {
  class EcdsaProvider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::runtime {
  class BeefyApi;
}

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::network {
  class BeefyProtocol;
  class BeefyThreadPool;

  class BeefyImpl : public Beefy,
                    public std::enable_shared_from_this<BeefyImpl> {
   public:
    BeefyImpl(application::AppStateManager &app_state_manager,
              const application::ChainSpec &chain_spec,
              std::shared_ptr<blockchain::BlockTree> block_tree,
              std::shared_ptr<runtime::BeefyApi> beefy_api,
              std::shared_ptr<crypto::EcdsaProvider> ecdsa,
              std::shared_ptr<storage::SpacedStorage> db,
              std::shared_ptr<common::MainPoolHandler> main_thread_handler,
              BeefyThreadPool &beefy_thread_pool,
              std::shared_ptr<libp2p::basic::Scheduler> scheduler,
              LazySPtr<consensus::Timeline> timeline,
              std::shared_ptr<crypto::SessionKeys> session_keys,
              LazySPtr<BeefyProtocol> beefy_protocol,
              primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);

    void prepare();
    void start();

    primitives::BlockNumber finalized() const override;

    outcome::result<std::optional<consensus::beefy::BeefyJustification>>
    getJustification(primitives::BlockNumber block) const override;

    void onJustification(const primitives::BlockHash &block_hash,
                         primitives::Justification raw) override;

    void onMessage(consensus::beefy::BeefyGossipMessage message) override;

   private:
    struct Session {
      consensus::beefy::ValidatorSet validators;
      std::map<primitives::BlockNumber, consensus::beefy::SignedCommitment>
          rounds;
    };
    using Sessions = std::map<primitives::BlockNumber, Session>;

    bool hasJustification(primitives::BlockNumber block) const;
    using FindValidatorsResult = std::optional<
        std::pair<primitives::BlockNumber, consensus::beefy::ValidatorSet>>;
    outcome::result<FindValidatorsResult> findValidators(
        primitives::BlockNumber max, primitives::BlockNumber min) const;
    outcome::result<void> onJustificationOutcome(
        const primitives::BlockHash &block_hash, primitives::Justification raw);
    outcome::result<void> onJustification(
        consensus::beefy::SignedCommitment justification);
    void onMessageStrand(consensus::beefy::BeefyGossipMessage message);
    void onVote(consensus::beefy::VoteMessage vote, bool broadcast);
    outcome::result<void> apply(
        consensus::beefy::SignedCommitment justification, bool broadcast);
    outcome::result<void> update();
    outcome::result<void> vote();
    outcome::result<std::optional<consensus::beefy::Commitment>> getCommitment(
        consensus::beefy::AuthoritySetId validator_set_id,
        primitives::BlockNumber block_number);
    void metricValidatorSetId();
    void broadcast(consensus::beefy::BeefyGossipMessage message);
    void setTimer();

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::BeefyApi> beefy_api_;
    std::shared_ptr<crypto::EcdsaProvider> ecdsa_;
    std::shared_ptr<storage::BufferStorage> db_;
    std::shared_ptr<common::MainPoolHandler> main_pool_handler_;
    std::shared_ptr<PoolHandler> beefy_pool_handler_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    LazySPtr<consensus::Timeline> timeline_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    LazySPtr<BeefyProtocol> beefy_protocol_;
    primitives::BlockNumber min_delta_;
    primitives::events::ChainSub chain_sub_;

    std::optional<primitives::BlockNumber> beefy_genesis_;
    primitives::BlockNumber beefy_finalized_ = 0;
    primitives::BlockNumber next_digest_ = 0;
    primitives::BlockNumber last_voted_ = 0;
    std::optional<consensus::beefy::VoteMessage> last_vote_;
    Sessions sessions_;
    std::map<primitives::BlockNumber, consensus::beefy::SignedCommitment>
        pending_justifications_;
    libp2p::basic::Scheduler::Handle timer_;

    log::Logger log_;
  };
}  // namespace kagome::network
