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

    /**
     * Prepares for grandpa round execution: e.g. sets justification observer
     * handler.
     * @return true preparation was done with no issues
     * @see kagome::application::AppStateManager::takeControl()
     */
    bool prepare();

    /**
     * Initiates grandpa voting process e.g.:
     *  - Obtains latest completed round state represented by MovableRoundState
     *  - Obtains authority set corresponding to the latest completed round
     *  - Uses obtained data to create and execute initial round
     * @return true if grandpa was executed
     * @see kagome::application::AppStateManager::takeControl()
     */
    bool start();

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
                           const network::GrandpaNeighborMessage &msg) override;

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
                          const network::CatchUpRequest &msg) override;

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
                           const network::CatchUpResponse &msg) override;

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
                       const network::VoteMessage &msg) override;

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
                         const network::FullCommitMessage &msg) override;

    /**
     * Selects round that corresponds for justification, checks justification,
     * finalizes corresponding block and stores justification in storage
     *
     * If there is no corresponding round, it will be created
     * @param block_info block being finalized by justification
     * @param justification justification containing precommit votes and
     * signatures for block info
     * @return nothing or an error
     */
    outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const GrandpaJustification &justification) override;

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
    /**
     * Selects round by provided number and voter set id
     * @param round_number number of round to be selected
     * @param voter_set_id  id of voter set for corresponding round
     * @return optional<shared_ptr> containing the round if we have one and
     * nullopt otherwise
     */
    std::optional<std::shared_ptr<VotingRound>> selectRound(
        RoundNumber round_number,
        std::optional<VoterSetId> voter_set_id);

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
        const MovableRoundState &round_state, std::shared_ptr<VoterSet> voters);

    /**
     * Takes given round and creates next one for it
     * @param previous_round VotingRound from which the new one is created
     * @return new VotingRound
     */
    std::shared_ptr<VotingRound> makeNextRound(
        const std::shared_ptr<VotingRound> &previous_round);

    /**
     * Request blocks that are missing to run consensus (for example when we
     * cannot accept precommit when there is no corresponding block)
     */
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

    libp2p::basic::Scheduler::Handle fallback_timer_handle_;

    log::Logger logger_ = log::createLogger("Grandpa", "grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
