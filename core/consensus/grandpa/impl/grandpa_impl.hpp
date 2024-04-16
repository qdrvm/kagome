/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"

#include <libp2p/basic/scheduler.hpp>

#include "consensus/grandpa/impl/votes_cache.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"
#include "utils/safe_object.hpp"

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

    /**
     * Processes grandpa neighbour message
     * Neighbour message is ignored if voter set ids between our and receiving
     * peer do not match. Otherwsie if our peer is behind by grandpa rounds by
     * more than GrandpaImpl::kCatchUpThreshold, then catch request is sent to
     * the peer that sent us a message (see GrandpaImpl::onCatchUpRequest()).
     *
     * Otherwise our peer will send back the response containing known votes for
     * the round in msg (if any)
     * @param peer_id id of the peer that sent the message
     * @param msg received grandpa neighbour message
     */
    void onNeighborMessage(const libp2p::peer::PeerId &peer_id,
                           std::optional<network::PeerStateCompact> &&info_opt,
                           network::GrandpaNeighborMessage &&msg) override;

    // Catch-up methods

    /**
     * Catch up request processing according to
     * [spec](https://w3f.github.io/polkadot-spec/develop/sect-block-finalization.html#algo-process-catchup-request)
     *
     * We check voter set ids between ours and remote peer match. Then we check
     * politeness of request and send response containing state for requested
     * round
     * @param peer_id id of the peer that sent catch up request
     * @param msg network message containing catch up request
     */
    void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                          std::optional<network::PeerStateCompact> &&info,
                          network::CatchUpRequest &&msg) override;

    /**
     * Catch up response processing according to
     * [spec](https://w3f.github.io/polkadot-spec/develop/sect-block-finalization.html#algo-process-catchup-response)
     *
     * Response is ignored if remote peer's voter set id does not match ours
     * peer id Response is ignored if message contains info about round in the
     * past (earlier than our current round) If message is received from the
     * future round we create round from the round state information in
     * response, check round for completeness and execute the round following
     * the round from response
     * @param peer_id id of remote peer that sent catch up response
     * @param msg message containing catch up response
     */
    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           network::CatchUpResponse &&msg) override;

    // Voting methods

    /**
     * Processing of vote messages
     *
     * Vote messages are ignored if they are not sent politely
     * Otherwise, we check if we have the round that corresponds to the received
     * message and process it by this round
     * @param peer_id id of remote peer
     * @param msg vote message that could be either primary propose, prevote, or
     * precommit message
     */
    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       std::optional<network::PeerStateCompact> &&info_opt,
                       network::VoteMessage &&msg) override;

    /**
     * Processing of commit message
     *
     * We check commit message for politeness and send it to
     * GrandpaImpl::applyJustification()
     *
     * @param peer_id id of remote peer
     * @param msg message containing commit message with justification
     */
    void onCommitMessage(const libp2p::peer::PeerId &peer_id,
                         network::FullCommitMessage &&msg) override;

    /**
     * Check justification votes signatures, ancestry and threshold.
     */
    outcome::result<void> verifyJustification(
        const GrandpaJustification &justification,
        const AuthoritySet &authorities) override;

    /**
     * Selects round that corresponds for justification, checks justification,
     * finalizes corresponding block and stores justification in storage
     *
     * If there is no corresponding round, it will be created
     * @param justification justification containing precommit votes and
     * signatures for block info
     * @return nothing or an error
     */
    void applyJustification(const GrandpaJustification &justification,
                            ApplyJustificationCb &&callback) override;

    void reload() override;

    // Round processing method

    /**
     * Creates and executes round that follows the round with provided
     * round_number
     *
     * Also this method removes old round, so that only 3 rounds in total are
     * stored at any moment
     * @param round previous round from which new one is created and executed
     */
    void tryExecuteNextRound(
        const std::shared_ptr<VotingRound> &round) override;

    /**
     * Selects round next to provided one and updates it by checking if
     * prevote_ghost, estimate and finalized block were updated.
     * @see VotingRound::update()
     * @param round_number the round after which we select the round for update
     */
    void updateNextRound(RoundNumber round_number) override;

   private:
    struct WaitingBlock {
      libp2p::peer::PeerId peer;
      boost::variant<network::VoteMessage,
                     network::CatchUpResponse,
                     network::FullCommitMessage>
          msg;
      MissingBlocks blocks;
    };

    void callbackCall(ApplyJustificationCb &&callback,
                      outcome::result<void> &&result);
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
     * @return Get grandpa::MovableRoundState for the last completed round
     */
    outcome::result<MovableRoundState> getLastCompletedRound() const;

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

    void saveCachedVotes();
    void applyCachedVotes(VotingRound &round);

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
    std::optional<
        const std::tuple<libp2p::peer::PeerId, network::CatchUpRequest>>
        pending_catchup_request_;
    libp2p::basic::Scheduler::Handle catchup_request_timer_handle_;
    libp2p::basic::Scheduler::Handle fallback_timer_handle_;

    std::vector<WaitingBlock> waiting_blocks_;

    struct CachedRound {
      SCALE_TIE(3);
      AuthoritySetId set;
      RoundNumber round;
      VotingRound::Votes votes;
    };
    using CachedVotes = std::vector<CachedRound>;
    CachedVotes cached_votes_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_highest_round_;
  };

}  // namespace kagome::consensus::grandpa
