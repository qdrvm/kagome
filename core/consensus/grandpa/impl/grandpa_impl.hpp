/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"

#include <libp2p/basic/scheduler.hpp>

#include "consensus/grandpa/historical_votes.hpp"
#include "consensus/grandpa/impl/votes_cache.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/lru.hpp"

namespace kagome {
  class PoolHandler;
  class PoolHandlerReady;
}  // namespace kagome

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

namespace kagome::consensus::grandpa {
  class AuthorityManager;
  class Environment;
  struct MovableRoundState;
  class VoterSet;
  class GrandpaThreadPool;
}  // namespace kagome::consensus::grandpa

namespace kagome::crypto {
  class Ed25519Provider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::network {
  class PeerManager;
  class ReputationRepository;
  class Synchronizer;
}  // namespace kagome::network

namespace kagome::consensus::grandpa {

  // clang-format off
  /*!
   * Main entry point to grandpa consensus implementation in KAGOME:
   * - starts grandpa consensus execution
   * - keeps track of current rounds
   *   - we should be able to receive votes for current and previous rounds
   * - processes incoming grandpa messages
   *   - we are checking if it is polite to process incoming messages and only then process it
   *        (see @link kagome::consensus::grandpa::RoundObserver#onVoteMessage() RoundObserver::onVoteMessage @endlink for more details)
   *  - processes rounds catch ups
   *
   * GrandpaImpl entity is registered in kagome::application::AppStateManager,
   * which executes prepare(), start() and stop() functions to manage execution of consensus algorithm
   *
   * When start is invoked we execute new grandpa round and gossip neighbour messages.
   *
   * Also we start receiving other grandpa messages over `paritytech/grandpa/1` protocol (see kagome::network::GrandpaProtocol::start()).
   * If we notice that we are lagging behind the most recent grandpa round in the network, then catch up procedure is executed (see GrandpaImpl::onNeighborMessage() and EnvironmentImpl::onCatchUpRequested()).
   * Catch up process is also described in the [spec](https://w3f.github.io/polkadot-spec/develop/sect-block-finalization.html#sect-grandpa-catchup)
   *
   * Otherwise if no catch up is needed, we proceed executing grandpa round until it is completable and previous round has finalized block
   */
  // clang-format on

  class GrandpaImpl : public Grandpa,
                      public GrandpaObserver,
                      public std::enable_shared_from_this<GrandpaImpl> {
   public:
    /// Maximum number of rounds we are behind a peer before issuing a catch up
    /// request.
    static const size_t kCatchUpThreshold = 2;

    /// Maximum number of rounds we are keep to communication
    static const size_t kKeepRecentRounds = 3;

    /// Timeout of waiting catchup response for request
    static constexpr Clock::Duration kCatchupRequestTimeout =
        std::chrono::milliseconds(45'000);

    ~GrandpaImpl() override = default;

    GrandpaImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<Environment> environment,
        std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<AuthorityManager> authority_manager,
        std::shared_ptr<network::Synchronizer> synchronizer,
        std::shared_ptr<network::PeerManager> peer_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<network::ReputationRepository> reputation_repository,
        LazySPtr<Timeline> timeline,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        storage::SpacedStorage &db,
        common::MainThreadPool &main_thread_pool,
        GrandpaThreadPool &grandpa_thread_pool);

    /**
     * Initiates grandpa voting process e.g.:
     *  - Obtains latest completed round state represented by MovableRoundState
     *  - Obtains authority set corresponding to the latest completed round
     *  - Uses obtained data to create and execute initial round
     * @return true if grandpa was executed
     */
    bool tryStart();

    /**
     * Does nothing. Needed only for AppStateManager
     * @see kagome::application::AppStateManager::takeControl()
     */
    void stop();

    // NeighborObserver
    void onNeighborMessage(const libp2p::peer::PeerId &peer_id,
                           std::optional<network::PeerStateCompact> &&info_opt,
                           network::GrandpaNeighborMessage &&msg) override;

    // CatchUpObserver
    void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                          std::optional<network::PeerStateCompact> &&info,
                          network::CatchUpRequest &&msg) override;
    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           network::CatchUpResponse &&msg) override;

    // RoundObserver
    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       std::optional<network::PeerStateCompact> &&info_opt,
                       network::VoteMessage &&msg) override;
    void onCommitMessage(const libp2p::peer::PeerId &peer_id,
                         network::FullCommitMessage &&msg) override;

    // JustificationObserver
    outcome::result<void> verifyJustification(
        const GrandpaJustification &justification,
        const AuthoritySet &authorities) override;
    void applyJustification(const GrandpaJustification &justification,
                            ApplyJustificationCb &&callback) override;
    void reload() override;

    // Grandpa
    void tryExecuteNextRound(
        const std::shared_ptr<VotingRound> &round) override;
    void updateNextRound(RoundNumber round_number) override;

    // SaveHistoricalVotes
    void saveHistoricalVote(AuthoritySetId set,
                            RoundNumber round,
                            const SignedMessage &vote,
                            bool set_index) override;

   private:
    struct WaitingBlock {
      libp2p::peer::PeerId peer;
      boost::variant<network::VoteMessage,
                     network::CatchUpResponse,
                     network::FullCommitMessage>
          msg;
      MissingBlocks blocks;
    };

    void startCurrentRound();

    void callbackCall(ApplyJustificationCb &&callback,
                      const outcome::result<void> &result);
    /**
     * Selects round by provided number and voter set id
     * @param round_number number of round to be selected
     * @param voter_set_id  id of voter set for corresponding round
     * @return optional<shared_ptr> containing the round if we have one and
     * nullopt otherwise
     */
    std::optional<std::shared_ptr<VotingRound>> selectRound(
        RoundNumber round_number, std::optional<VoterSetId> voter_set_id);

    /**
     * Make next round from last saved justification.
     */
    outcome::result<void> makeRoundAfterLastFinalized();

    /**
     * Initializes new round by provided round state and voter. Note that round
     * created that way is instantly ended. Because it is needed only to become
     * basis of the next round
     * @param round_state information required to execute the round
     * @param voters is the set of voters for this round
     * @return VotingRound that we can execute
     */
    std::shared_ptr<VotingRound> makeInitialRound(
        const MovableRoundState &round_state,
        std::shared_ptr<VoterSet> voters,
        const AuthoritySet &authority_set);

    /**
     * Takes given round and creates next one for it
     * @param previous_round VotingRound from which the new one is created
     * @return new VotingRound or error
     */
    outcome::result<std::shared_ptr<VotingRound>> makeNextRound(
        const std::shared_ptr<VotingRound> &previous_round);

    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           network::CatchUpResponse &&msg,
                           bool allow_missing_blocks);
    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       std::optional<network::PeerStateCompact> &&info_opt,
                       network::VoteMessage &&msg,
                       bool allow_missing_blocks);
    void onCommitMessage(const libp2p::peer::PeerId &peer_id,
                         network::FullCommitMessage &&msg,
                         bool allow_missing_blocks);
    /**
     * Request blocks that are missing to run consensus (for example when we
     * cannot accept precommit when there is no corresponding block)
     */
    void loadMissingBlocks(WaitingBlock &&waiting);
    void onHead(const primitives::BlockInfo &block);
    void pruneWaitingBlocks();

    void writeHistoricalVotes();
    using HistoricalVotesDirty = std::pair<HistoricalVotes, bool>;
    HistoricalVotesDirty &historicalVotes(AuthoritySetId set,
                                          RoundNumber round);
    void applyHistoricalVotes(VotingRound &round);

    void setTimerFallback();

    log::Logger logger_ = log::createLogger("Grandpa", "grandpa");

    const size_t kVotesCacheSize = 5;

    const Clock::Duration round_time_factor_;

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<AuthorityManager> authority_manager_;
    std::shared_ptr<network::Synchronizer> synchronizer_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    VotesCache votes_cache_{kVotesCacheSize};
    std::shared_ptr<network::ReputationRepository> reputation_repository_;
    LazySPtr<Timeline> timeline_;
    primitives::events::ChainSub chain_sub_;
    std::shared_ptr<storage::BufferStorage> db_;

    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<PoolHandlerReady> grandpa_pool_handler_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::shared_ptr<VotingRound> current_round_;
    std::map<libp2p::peer::PeerId, network::CatchUpRequest>
        pending_catchup_request_;
    libp2p::basic::Scheduler::Handle catchup_request_timer_handle_;
    libp2p::basic::Scheduler::Handle fallback_timer_handle_;

    std::vector<WaitingBlock> waiting_blocks_;

    using HistoricalVotesKey = std::pair<AuthoritySetId, RoundNumber>;
    Lru<HistoricalVotesKey,
        HistoricalVotesDirty,
        boost::hash<HistoricalVotesKey>>
        historical_votes_{5};
    bool writing_historical_votes_ = false;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_highest_round_;
  };

}  // namespace kagome::consensus::grandpa
