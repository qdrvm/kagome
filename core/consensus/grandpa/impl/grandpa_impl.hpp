/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
#define KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL

#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"

#include <boost/operators.hpp>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/peer_manager.hpp"
#include "network/synchronizer.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"

namespace kagome::consensus::grandpa {

  class GrandpaImpl : public Grandpa,
                      public GrandpaObserver,
                      public std::enable_shared_from_this<GrandpaImpl> {
   public:
    /// Maximum number of rounds we are behind a peer before issuing a catch up
    /// request.
    static const size_t kCatchUpThreshold = 2;

    /// Maximum number of rounds we are keep to communication
    static const size_t kKeepRecentRounds = 3;

    ~GrandpaImpl() override = default;

    GrandpaImpl(std::shared_ptr<application::AppStateManager> app_state_manager,
                std::shared_ptr<Environment> environment,
                std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
                std::shared_ptr<runtime::GrandpaApi> grandpa_api,
                const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
                std::shared_ptr<Clock> clock,
                std::shared_ptr<libp2p::basic::Scheduler> scheduler,
                std::shared_ptr<authority::AuthorityManager> authority_manager,
                std::shared_ptr<network::Synchronizer> synchronizer,
                std::shared_ptr<network::PeerManager> peer_manager,
                std::shared_ptr<blockchain::BlockTree> block_tree);

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    // Neighbor

    void onNeighborMessage(const libp2p::peer::PeerId &peer_id,
                           const network::GrandpaNeighborMessage &msg) override;

    // Catch-up methods

    void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                          const network::CatchUpRequest &msg) override;

    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           const network::CatchUpResponse &msg) override;

    // Voting methods

    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       const network::VoteMessage &msg) override;

    void onCommitMessage(const libp2p::peer::PeerId &peer_id,
                         const network::FullCommitMessage &msg) override;

    outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const GrandpaJustification &justification) override;

    // Round processing method

    void executeNextRound(RoundNumber round_number) override;

   private:
    std::shared_ptr<VotingRound> selectRound(
        RoundNumber round_number,
        std::optional<MembershipCounter> voter_set_id);
    outcome::result<MovableRoundState> getLastCompletedRound() const;

    std::shared_ptr<VotingRound> makeInitialRound(
        const MovableRoundState &round_state, std::shared_ptr<VoterSet> voters);

    std::shared_ptr<VotingRound> makeNextRound(
        const std::shared_ptr<VotingRound> &previous_round);

    void loadMissingBlocks();

    // Note: Duration value was gotten from substrate
    // https://github.com/paritytech/substrate/blob/efbac7be80c6e8988a25339061078d3e300f132d/bin/node-template/node/src/service.rs#L166
    // Perhaps, 333ms is not enough for normal communication during the round
    const Clock::Duration round_time_factor_ = std::chrono::milliseconds(333);

    std::shared_ptr<VotingRound> current_round_;

    std::shared_ptr<Environment> environment_;
    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    const std::shared_ptr<crypto::Ed25519Keypair> &keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<authority::AuthorityManager> authority_manager_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_highest_round_;

    log::Logger logger_ = log::createLogger("Grandpa", "grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
