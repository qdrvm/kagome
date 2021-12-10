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
#include "network/synchronizer.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus::grandpa {

  // help struct to correctly compare rounds in different voter sets
  struct FullRound : boost::less_than_comparable<FullRound>,
                     boost::equality_comparable<FullRound> {
    MembershipCounter voter_set_id;
    RoundNumber round_number;
    explicit FullRound(const std::shared_ptr<const VotingRound> &round)
        : voter_set_id(round->voterSetId()),
          round_number(round->roundNumber()) {}
    explicit FullRound(const network::GrandpaNeighborMessage &msg)
        : voter_set_id(msg.voter_set_id), round_number(msg.round_number) {}
    explicit FullRound(const network::CatchUpRequest &msg)
        : voter_set_id(msg.voter_set_id), round_number(msg.round_number) {}
    explicit FullRound(const network::CatchUpResponse &msg)
        : voter_set_id(msg.voter_set_id), round_number(msg.round_number) {}
    explicit FullRound(const VoteMessage &msg)
        : voter_set_id(msg.counter), round_number(msg.round_number) {}
    FullRound &operator=(const FullRound &) = default;

    bool operator<(const FullRound &round) const {
      return voter_set_id < round.voter_set_id
             or (voter_set_id == round.voter_set_id
                 and round_number < round.round_number);
    }

    bool operator==(const FullRound &round) const {
      return voter_set_id == round.voter_set_id
             && round_number == round.round_number;
    }
  };

  class GrandpaImpl : public Grandpa,
                      public GrandpaObserver,
                      public std::enable_shared_from_this<GrandpaImpl> {
   public:
    ~GrandpaImpl() override = default;

    GrandpaImpl(std::shared_ptr<application::AppStateManager> app_state_manager,
                std::shared_ptr<Environment> environment,
                std::shared_ptr<storage::BufferStorage> storage,
                std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
                std::shared_ptr<runtime::GrandpaApi> grandpa_api,
                const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
                std::shared_ptr<Clock> clock,
                std::shared_ptr<boost::asio::io_context> io_context,
                std::shared_ptr<authority::AuthorityManager> authority_manager,
                std::shared_ptr<network::Synchronizer> synchronizer);

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

    void executeNextRound() override;

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

    void onCompletedRound(outcome::result<MovableRoundState> round_state_res);

    void tryCatchUp(const libp2p::peer::PeerId &peer_id,
                    const FullRound &next,
                    const FullRound &curr);

    // Note: Duration value was gotten from substrate
    // https://github.com/paritytech/substrate/blob/efbac7be80c6e8988a25339061078d3e300f132d/bin/node-template/node/src/service.rs#L166
    // Perhaps, 333ms is not enough for normal communication during the round
    const Clock::Duration round_time_factor_ = std::chrono::milliseconds(333);

    std::shared_ptr<VotingRound> previous_round_;
    std::shared_ptr<VotingRound> current_round_;

    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    const std::shared_ptr<crypto::Ed25519Keypair> &keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<authority::AuthorityManager> authority_manager_;
    std::shared_ptr<network::Synchronizer> synchronizer_;

    std::vector<FullRound> neighbor_msgs_{};

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_highest_round_;

    log::Logger logger_ = log::createLogger("Grandpa", "grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
