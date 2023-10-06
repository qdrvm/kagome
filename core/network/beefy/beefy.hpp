/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context_strand.hpp>
#include <map>

#include "consensus/beefy/types.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "network/beefy/i_beefy.hpp"
#include "primitives/event_types.hpp"
#include "primitives/justification.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome {
  class ThreadPool;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::consensus {
  class Timeline;
}  // namespace kagome::consensus

namespace kagome::crypto {
  class EcdsaProvider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::runtime {
  class BeefyApi;
}  // namespace kagome::runtime

namespace kagome::storage {
  class SpacedStorage;
}  // namespace kagome::storage

namespace kagome::network {
  class BeefyProtocol;

  class Beefy : public IBeefy, public std::enable_shared_from_this<Beefy> {
   public:
    Beefy(application::AppStateManager &app_state_manager,
          const application::ChainSpec &chain_spec,
          std::shared_ptr<blockchain::BlockTree> block_tree,
          std::shared_ptr<runtime::BeefyApi> beefy_api,
          std::shared_ptr<crypto::EcdsaProvider> ecdsa,
          std::shared_ptr<storage::SpacedStorage> db,
          std::shared_ptr<ThreadPool> thread_pool,
          std::shared_ptr<boost::asio::io_context> main_thread,
          std::shared_ptr<consensus::Timeline> timeline,
          std::shared_ptr<crypto::SessionKeys> session_keys,
          LazySPtr<BeefyProtocol> beefy_protocol,
          std::shared_ptr<primitives::events::ChainSubscriptionEngine>
              chain_sub_engine);

    primitives::BlockNumber finalized() const;

    outcome::result<std::optional<consensus::beefy::BeefyJustification>>
    getJustification(primitives::BlockNumber block) const override;

    void onJustification(const primitives::BlockHash &block_hash,
                         primitives::Justification raw) override;

    void onMessage(consensus::beefy::BeefyGossipMessage message);

   private:
    struct Session {
      consensus::beefy::ValidatorSet validators;
      std::map<primitives::BlockNumber, consensus::beefy::SignedCommitment>
          rounds;
    };
    using Sessions = std::map<primitives::BlockNumber, Session>;

    void start(std::shared_ptr<primitives::events::ChainSubscriptionEngine>
                   chain_sub_engine);
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
    void metricValidatorSetId();

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::BeefyApi> beefy_api_;
    std::shared_ptr<crypto::EcdsaProvider> ecdsa_;
    std::shared_ptr<storage::BufferStorage> db_;
    std::shared_ptr<boost::asio::io_context> strand_inner_;
    boost::asio::io_context::strand strand_;
    std::shared_ptr<boost::asio::io_context> main_thread_;
    std::shared_ptr<consensus::Timeline> timeline_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    LazySPtr<BeefyProtocol> beefy_protocol_;
    primitives::BlockNumber min_delta_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    std::optional<primitives::BlockNumber> beefy_genesis_;
    primitives::BlockNumber beefy_finalized_ = 0;
    primitives::BlockNumber next_digest_ = 0;
    primitives::BlockNumber last_voted_ = 0;
    Sessions sessions_;
    std::map<primitives::BlockNumber, consensus::beefy::SignedCommitment>
        pending_justifications_;

    log::Logger log_;
  };
}  // namespace kagome::network
