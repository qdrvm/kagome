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
  class PoolHandlerReady;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::consensus {
  class Timeline;
}
namespace kagome::consensus::beefy {
  class FetchJustification;
}  // namespace kagome::consensus::beefy

namespace kagome::crypto {
  class EcdsaProvider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::offchain {
  class OffchainWorkerFactory;
  class OffchainWorkerPool;
}  // namespace kagome::offchain

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
    BeefyImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const application::ChainSpec &chain_spec,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::BeefyApi> beefy_api,
        std::shared_ptr<crypto::EcdsaProvider> ecdsa,
        std::shared_ptr<storage::SpacedStorage> db,
        common::MainThreadPool &main_thread_pool,
        BeefyThreadPool &beefy_thread_pool,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        LazySPtr<consensus::Timeline> timeline,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        LazySPtr<BeefyProtocol> beefy_protocol,
        LazySPtr<consensus::beefy::FetchJustification>
            beefy_justification_protocol,
        std::shared_ptr<offchain::OffchainWorkerFactory>
            offchain_worker_factory,
        std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);

    bool tryStart();

    primitives::BlockNumber finalized() const override;

    outcome::result<std::optional<consensus::beefy::BeefyJustification>>
    getJustification(primitives::BlockNumber block) const override;

    void onJustification(const primitives::BlockHash &block_hash,
                         primitives::Justification raw) override;

    void onMessage(consensus::beefy::BeefyGossipMessage message) override;

   private:
    struct DoubleVoting {
      consensus::beefy::VoteMessage first;
      bool reported = false;
    };
    struct Round {
      // https://github.com/paritytech/polkadot-sdk/blob/efdc1e9b1615c5502ed63ffc9683d99af6397263/substrate/client/consensus/beefy/src/round.rs#L87
      std::unordered_map<consensus::beefy::Commitment,
                         consensus::beefy::SignedCommitment>
          justifications;
      // https://github.com/paritytech/polkadot-sdk/blob/efdc1e9b1615c5502ed63ffc9683d99af6397263/substrate/client/consensus/beefy/src/round.rs#L88
      std::unordered_map<size_t, DoubleVoting> double_voting;
    };
    struct Session {
      consensus::beefy::ValidatorSet validators;
      std::map<primitives::BlockNumber, Round> rounds;
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
    outcome::result<void> reportDoubleVoting(
        const consensus::beefy::DoubleVotingProof &votes);

    log::Logger log_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::BeefyApi> beefy_api_;
    std::shared_ptr<crypto::EcdsaProvider> ecdsa_;
    std::shared_ptr<storage::BufferStorage> db_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<PoolHandlerReady> beefy_pool_handler_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    LazySPtr<consensus::Timeline> timeline_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    LazySPtr<BeefyProtocol> beefy_protocol_;
    LazySPtr<consensus::beefy::FetchJustification>
        beefy_justification_protocol_;
    std::shared_ptr<offchain::OffchainWorkerFactory> offchain_worker_factory_;
    std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool_;
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
  };
}  // namespace kagome::network
